#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <zmq.hpp>
#include "postProcessor.h"
#include "ImageStructure.hpp"
#include "utils.h"

namespace fs = std::filesystem;

// Constructor with default parameters
PostProcessor::PostProcessor(int bufferSize, int timeoutMs, const std::string &outputDir)
    : maxFrames(bufferSize),
      currentFrameIndex(0),
      firstRun(true),
      currentlyFilling(BufferPart::FIRST),
      isRunning(false),
      timeoutDuration(timeoutMs),
      outputDirectory(outputDir)
{
    // Create output directory if it doesn't exist
    if (!outputDirectory.empty())
    {
        try
        {
            fs::create_directories(outputDirectory);
            std::cout << "Video output directory: " << outputDirectory << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "Error creating directory " << outputDirectory
                      << ": " << e.what() << std::endl;
            // Fallback to current directory
            outputDirectory = ".";
        }
    }
    else
    {
        outputDirectory = "."; // Save to current directory
    }

    // Ensure buffer size is divisible by 3
    if (maxFrames % 3 != 0)
    {
        // Round up to the nearest multiple of 3
        maxFrames = ((maxFrames / 3) + 1) * 3;
        std::cout << "Buffer size adjusted to " << maxFrames
                  << " (must be divisible by 3)" << std::endl;
    }

    // Calculate size of one buffer segment (1/3 of total)
    bufferPartSize = maxFrames / 3;

    // Initialize frame buffer
    frameBuffer.resize(maxFrames);
    initializeBuffer();

    // Initialize last frame timestamp to now
    lastFrameTime = std::chrono::steady_clock::now();

    std::cout << "PostProcessor initialized with buffer size: "
              << maxFrames << " frames" << std::endl;
    std::cout << "Size of one segment: " << bufferPartSize << " frames" << std::endl;
}

// Destructor
PostProcessor::~PostProcessor()
{
    stop(); // Stop processing before destruction
}

// Initialize buffer with black frames
void PostProcessor::initializeBuffer()
{
    // Iterate over all buffer slots
    for (int i = 0; i < maxFrames; i++)
    {
        // Create a black placeholder frame (will be updated on first real frame)
        frameBuffer[i].frame = cv::Mat::zeros(480, 640, CV_8UC3);
        frameBuffer[i].index = -1; // Index -1 means "empty"
    }
    std::cout << "Buffer initialized with black frames" << std::endl;
}

// Create a black frame of specified dimensions
cv::Mat PostProcessor::createBlackFrame(int width, int height)
{
    // Create a zero-filled matrix (black image)
    // CV_8UC3 = 8-bit unsigned, 3 channels (BGR)
    return cv::Mat::zeros(height, width, CV_8UC3);
}

// Add a new frame to the circular buffer
void PostProcessor::addFrame(const cv::Mat &frame, int index)
{
    std::lock_guard<std::mutex> lock(bufferMutex); // Thread-safe access

    // Skip empty frames
    if (frame.empty())
    {
        std::cerr << "Warning: received empty frame with index " << index << std::endl;
        return;
    }

    // Update timestamp of last received frame
    lastFrameTime = std::chrono::steady_clock::now();

    // On first frame, adjust all black frames to match real frame size
    if (currentFrameIndex == 0)
    {
        for (int i = 0; i < maxFrames; i++)
        {
            if (frameBuffer[i].frame.size() != frame.size())
            {
                frameBuffer[i].frame = createBlackFrame(frame.cols, frame.rows);
            }
        }
    }

    // Use cyclic index based on frame ID
    int bufferIndex = index % maxFrames;
    frame.copyTo(frameBuffer[bufferIndex].frame);
    frameBuffer[bufferIndex].index = index;

    std::cout << "Frame " << index << " added to buffer at position " << bufferIndex << std::endl;

    // Check if a buffer segment is full and needs saving
    checkAndSaveIfNeeded();
}

// Check if a buffer segment is full and trigger saving
void PostProcessor::checkAndSaveIfNeeded()
{
    if (firstRun)
    {
        // On first run, wait until the third segment starts filling
        if (currentFrameIndex >= bufferPartSize * 2)
        {
            std::cout << "First two segments filled, starting third segment" << std::endl;
            currentlyFilling = BufferPart::THIRD;
            savePartToVideo(BufferPart::FIRST);
            firstRun = false;
        }
    }
    else
    {
        int currentPart = currentFrameIndex / bufferPartSize;

        // Detect segment boundary crossings
        if ((currentPart == 1 && currentlyFilling == BufferPart::FIRST) ||
            (currentPart == 2 && currentlyFilling == BufferPart::SECOND) ||
            (currentPart == 0 && currentlyFilling == BufferPart::THIRD))
        {
            switch (currentPart)
            {
            case 0:
                currentlyFilling = BufferPart::FIRST;
                savePartToVideo(BufferPart::SECOND);
                break;
            case 1:
                currentlyFilling = BufferPart::SECOND;
                savePartToVideo(BufferPart::THIRD);
                break;
            case 2:
                currentlyFilling = BufferPart::THIRD;
                savePartToVideo(BufferPart::FIRST);
                currentFrameIndex = 0; // Reset for cyclic reuse
                break;
            }
        }
    }
}

// Save a buffer segment to a video file
void PostProcessor::savePartToVideo(BufferPart partToSave)
{
    std::cout << "Starting save operation" << std::endl;

    // Determine buffer indices for the segment
    int startIdx, endIdx;
    switch (partToSave)
    {
    case BufferPart::FIRST:
        startIdx = 0;
        endIdx = bufferPartSize - 1;
        break;
    case BufferPart::SECOND:
        startIdx = bufferPartSize;
        endIdx = bufferPartSize * 2 - 1;
        break;
    case BufferPart::THIRD:
        startIdx = bufferPartSize * 2;
        endIdx = maxFrames - 1;
        break;
    }

    // Check if segment contains any valid frames
    bool hasFrames = false;
    for (int i = startIdx; i <= endIdx; i++)
    {
        if (frameBuffer[i].index != -1)
        {
            hasFrames = true;
            break;
        }
    }

    std::cout << "Frames exist in segment: " << (hasFrames ? "true" : "false") << std::endl;

    if (!hasFrames)
    {
        std::cout << "No frames to save in segment " << static_cast<int>(partToSave) << std::endl;
        return;
    }

    // Copy frames to temporary vector for safe async processing
    std::vector<FrameWithIndex> framesToSave;
    std::string currentOutputDir;

    {
        std::cout << "Acquired mutex, copying frames" << std::endl;
        for (int i = startIdx; i <= endIdx; i++)
        {
            FrameWithIndex frameCopy;
            frameBuffer[i].frame.copyTo(frameCopy.frame);
            frameCopy.index = frameBuffer[i].index;
            framesToSave.push_back(frameCopy);
        }
        currentOutputDir = outputDirectory;

        // Reset saved segment to black frames
        resetBufferPart(partToSave);
    }

    std::cout << "Copied " << framesToSave.size() << " frames for saving" << std::endl;

    if (framesToSave.empty())
    {
        std::cout << "No frames to save after copying" << std::endl;
        return;
    }

    std::string localOutputDir = currentOutputDir;

    // Save in background thread to avoid blocking main loop
    std::thread saveThread([this, framesToSave, partToSave, localOutputDir]() mutable
    {
        try
        {
            std::cout << "Started save thread for segment " << static_cast<int>(partToSave) << std::endl;

            // Generate timestamped filename
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
#ifdef _WIN32
            localtime_s(&tm_now, &time_t_now);
#else
            localtime_r(&time_t_now, &tm_now);
#endif

            std::stringstream filename;
            filename << "video_part_" << static_cast<int>(partToSave) << "_"
                     << std::put_time(&tm_now, "%Y%m%d_%H%M%S") << ".avi";

            std::cout << "Output directory: " << localOutputDir << std::endl;

            std::string fullPath;
            if (!localOutputDir.empty() && localOutputDir != ".")
            {
                try
                {
                    if (!fs::exists(localOutputDir))
                    {
                        std::cout << "Creating directory: " << localOutputDir << std::endl;
                        fs::create_directories(localOutputDir);
                    }
                    fullPath = localOutputDir + "/" + filename.str();
                }
                catch (const fs::filesystem_error& e)
                {
                    std::cerr << "Error creating directory: " << e.what() << std::endl;
                    fullPath = filename.str();
                }
            }
            else
            {
                fullPath = filename.str();
            }

            std::cout << "Full path: " << fullPath << std::endl;

            // Write frames to video file
            saveFramesToVideo(framesToSave, fullPath);

            std::cout << "Save thread finished for segment " << static_cast<int>(partToSave) << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in save thread: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception in save thread" << std::endl;
        }
    });

    saveThread.detach();

    std::cout << "Detached save thread for segment " << static_cast<int>(partToSave)
              << ". Frames to save: " << framesToSave.size() << std::endl;

    std::cout << "Started saving segment " << static_cast<int>(partToSave)
              << " to file in: " << outputDirectory << std::endl;
}

// Write a sequence of frames to a video file
void PostProcessor::saveFramesToVideo(const std::vector<FrameWithIndex> &frames, const std::string &filename)
{
    if (frames.empty())
    {
        std::cerr << "No frames to save to file " << filename << std::endl;
        return;
    }

    try
    {
        std::cout << "Starting video writing" << std::endl;
        cv::Size frameSize = frames[0].frame.size();

        // Try XVID codec first
        cv::VideoWriter videoWriter(filename,
                                    cv::VideoWriter::fourcc('X', 'V', 'I', 'D'),
                                    30.0,
                                    frameSize);

        if (!videoWriter.isOpened())
        {
            std::cerr << "Error: failed to create video file " << filename << std::endl;
            std::cerr << "Trying MJPG codec..." << std::endl;
            videoWriter.open(filename,
                             cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                             30.0,
                             frameSize);

            if (!videoWriter.isOpened())
            {
                std::cerr << "Error: failed to create video file with MJPG codec" << std::endl;
                return;
            }
        }

        // Write all frames
        for (const auto &frameWithIndex : frames)
        {
            videoWriter.write(frameWithIndex.frame);
        }

        videoWriter.release();

        std::cout << "Video saved: " << filename
                  << " (frames: " << frames.size() << ")" << std::endl;
    }
    catch (const cv::Exception &e)
    {
        std::cerr << "OpenCV error while saving video: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error while saving video: " << e.what() << std::endl;
    }
}

// Reset a buffer segment to black frames
void PostProcessor::resetBufferPart(BufferPart part)
{
    int startIdx, endIdx;
    switch (part)
    {
    case BufferPart::FIRST:
        startIdx = 0;
        endIdx = bufferPartSize - 1;
        break;
    case BufferPart::SECOND:
        startIdx = bufferPartSize;
        endIdx = bufferPartSize * 2 - 1;
        break;
    case BufferPart::THIRD:
        startIdx = bufferPartSize * 2;
        endIdx = maxFrames - 1;
        break;
    }

    for (int i = startIdx; i <= endIdx; i++)
    {
        if (!frameBuffer[i].frame.empty())
        {
            int width = frameBuffer[i].frame.cols;
            int height = frameBuffer[i].frame.rows;
            frameBuffer[i].frame = createBlackFrame(width, height);
        }
        frameBuffer[i].index = -1;
    }

    std::cout << "Buffer segment " << static_cast<int>(part)
              << " reset to black frames" << std::endl;
}

// Force-save all remaining frames
void PostProcessor::flushAll()
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (currentFrameIndex > 0)
    {
        if (firstRun)
        {
            int filledParts = (currentFrameIndex - 1) / bufferPartSize + 1;
            for (int part = 0; part < filledParts; part++)
            {
                savePartToVideo(static_cast<BufferPart>(part));
            }
        }
        else
        {
            savePartToVideo(BufferPart::FIRST);
            savePartToVideo(BufferPart::SECOND);
            savePartToVideo(BufferPart::THIRD);
        }
    }

    std::cout << "All frames saved to directory: " << outputDirectory << std::endl;
}

// Background thread that checks for inactivity timeout
void PostProcessor::timeoutChecker()
{
    while (isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);

        if (elapsed > timeoutDuration)
        {
            std::cout << "Timeout expired, saving remaining frames..." << std::endl;
            flushAll();
            lastFrameTime = now; // Reset timer
        }
    }
}

// Start the post-processor (spawn timeout thread)
void PostProcessor::start()
{
    if (!isRunning)
    {
        isRunning = true;
        timeoutThread = std::thread(&PostProcessor::timeoutChecker, this);
        std::cout << "PostProcessor started. Videos saved to: " << outputDirectory << std::endl;
    }
}

// Stop the post-processor and save remaining data
void PostProcessor::stop()
{
    if (isRunning)
    {
        isRunning = false;
        if (timeoutThread.joinable())
        {
            timeoutThread.join();
        }
        flushAll();
        std::cout << "PostProcessor stopped" << std::endl;
    }
}

// Change output directory at runtime
void PostProcessor::setOutputDirectory(const std::string &dir)
{
    if (!dir.empty())
    {
        try
        {
            fs::create_directories(dir);
            outputDirectory = dir;
            std::cout << "Video output directory changed to: " << outputDirectory << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "Error creating directory " << dir
                      << ": " << e.what() << std::endl;
        }
    }
}

// Main entry point
int main() {
    std::cout << "=== POSTPROCESSOR (REAL MODE) STARTED ===" << std::endl;

#ifdef DEBUG
    std::cout << "DEBUG MODE ENABLED" << std::endl;
#endif

    // 1. Load configuration
    Utils config;
    config.loadConfig();

    int zmq_port = std::stoi(config.getConfig("postprocessor.zmq_port")); // e.g., 5557
    std::string output_dir = config.getConfig("postprocessor.output_dir");
    int max_frames = std::stoi(config.getConfig("postprocessor.max_frames"));
    int timeout_ms = std::stoi(config.getConfig("postprocessor.timeout_duration"));

    std::filesystem::create_directories(output_dir);

    // 2. Initialize post-processor
    PostProcessor videoProcessor(max_frames, timeout_ms, output_dir);
    videoProcessor.start();

    // 3. Set up ZeroMQ PULL socket to receive from worker
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::pull);
    std::string endpoint = "tcp://*:" + std::to_string(zmq_port);
    socket.bind(endpoint);
    std::cout << "Bound to: " << endpoint << std::endl;

    // 4. Main receive loop
    while (true) {
        zmq::message_t msg;
        auto result = socket.recv(msg, zmq::recv_flags::none);

        if (!result.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Deserialize frame
        ImageStructure is;
        if (!is.deserialize(msg)) {
            std::cerr << "Failed to deserialize incoming frame" << std::endl;
            continue;
        }

        cv::Mat frame = is.getImage();
        uint64_t frame_id = is.getFrameId();

        if (frame.empty()) {
            std::cerr << "Received empty frame" << std::endl;
            continue;
        }

        std::cout << "- [ OK ] Received frame " << frame_id
                  << " (" << frame.cols << "x" << frame.rows << ")" << std::endl;

        // Pass frame to post-processor
        videoProcessor.addFrame(frame, static_cast<int>(frame_id));
    }

    videoProcessor.stop();
    return 0;
}