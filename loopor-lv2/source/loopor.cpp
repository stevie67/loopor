//
// MIT License
//
// Copyright 2018 Stevie <modplugins@radig.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <math.h>
#include <stdlib.h>

// Needed for the callbacks
#include <functional>

// Needed for writing debug output to a log file
#include <stdarg.h>
#include <string.h>

// Core definitions for the LV2 interface
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

//
// Configuration constants
//

/// URI which identifies the plugin
static const char* LOOPER_URI = "http://radig.com/plugins/loopor";
/// The maximum number of dubs that can be recorded
static const size_t NR_OF_DUBS = 128;
/// The maximum number of seconds which can be recorded for all dubs.
/// Note that each dub can have an individual length. If audio starts
/// after the loop start and/or finishes before the end of the loop
/// it will consume less memory.
static const size_t STORAGE_MEMORY_SECONDS = 360;
static const size_t NR_OF_BLEND_SAMPLES = 64;
/// Allow to enable logging to a file (/root/loopor.log)
static const bool LOG_ENABLED = false;

///
/// Convert an input parameter expressed as db into a linear float value
///
static float dbToFloat(float db)
{
    if (db <= -90.0f)
        return 0.0f;
    return powf(10.0f, db * 0.05f);
}

///
/// Represent the state of the dub
///
typedef enum
{
    // The looper does not have an active dub and is not recording.
    LOOPER_STATE_INACTIVE,
    // The looper has started recording a dub, but audio did not exceed the
    // threshold so far.
    LOOPER_STATE_WAITING_FOR_THRESHOLD,
    // The looper is recording a dub.
    LOOPER_STATE_RECORDING,
    // The looper is still playing all the active dubs.
    LOOPER_STATE_PLAYING
} State;

///
/// The indices for the ports we support
///
enum PortIndex
{
    /// Audio input 1
    LOOPER_INPUT1 = 0,
    /// Audio input 2
    LOOPER_INPUT2 = 1,
    /// Audio output 1
    LOOPER_OUTPUT1 = 2,
    /// Audio output 2
    LOOPER_OUTPUT2 = 3,
    /// Threshold parameter
    LOOPER_THRESHOLD = 4,
    /// Activate button
    LOOPER_ACTIVATE = 5,
    /// Reset button
    LOOPER_RESET = 6,
    /// Undo button
    LOOPER_UNDO = 7,
    /// Redo button
    LOOPER_REDO = 8,
    /// Dub button
    LOOPER_DUB = 9,
    /// Amount of the dry signal in the output
    LOOPER_DRY_AMOUNT = 10,
    /// Select if dub ends at end of loop
    LOOPER_CONTINUOUS_DUB = 11,
};

///
/// Represent a dub
///
class Dub
{
public:
    /// Where is the dub's audio memory starting in the global audio storage?
    size_t m_storageOffset = 0;
    /// The length of the dub. Each dub can have an individual length, but they
    /// will still stay in sync!
    size_t m_length = 0;
    /// The start index in the loop. This allows to save the memory before there
    /// is actual audio in the loop. A dub only needs the memory between the
    /// first and the last audio saved in the dub.
    size_t m_startIndex = 0;
};

///
/// Simplify handling of momentary (aka trigger) buttons. It allows to connect to
/// a float LV2 input and will call a callback function when the value changes.
/// Also allows for double clicks without a second. Should the host allow it, it
/// would also allow for long / short press distinction.
///
class MomentaryButton
{
public:
    // Connect the button to an input and set the callback.
    void connect(void* input, std::function<void (bool, double, bool)> callback)
    {
        m_input = static_cast<const float*>(input);
        m_callback = callback;
    }

    /// To be called every run call of the plugin. Will check the state of the
    /// button and call the callback if necessary.
    ///
    /// \param now The current time. Used for checking for double clicks.
    void run(double now)
    {
        if (m_input == NULL)
            return;

        bool state = (*m_input) > 0.0f ? true : false;
        if (state == m_lastState)
            return;
        m_lastState = state;
        bool doubleClick = false;
        if (state)
        {
            if (now - m_lastClickTime < 1)
                doubleClick = true;
            m_lastClickTime = now;
        }
        m_callback(state, now - m_lastChangeTime, doubleClick);
        m_lastChangeTime = now;
    }

    /// The callback
    /// \param bool pressed Is the button pressed or released?
    /// \param double How long since the last state change?
    /// \param doubleClick Was this pressed twice within a second?
    std::function<void (bool, double, bool)> m_callback;
    /// The input it is connected to.
    const float* m_input = NULL;
    /// The last state, used for supressing multiple callbacks.
    bool m_lastState = false;
    /// When was the last change
    double m_lastChangeTime = 0;
    /// Used for detecting double clicks.
    double m_lastClickTime = 0;
};

///
/// The looper class
///
class Looper
{
public:
    /// Constructor
    /// \param sampleRate The sample rate is used for calculation of the storage needed
    ///                   and also the current time.
    Looper(double sampleRate)
        : m_sampleRate(sampleRate)
    {
        // Allocate the needed memory
        m_storageSize = sampleRate * STORAGE_MEMORY_SECONDS * 2;
        m_storage1 = new float[m_storageSize];
        m_storage2 = new float[m_storageSize];

        if (LOG_ENABLED)
            m_logFile = fopen("/root/loopor.log", "wb");
    }

    // Destructor
    ~Looper()
    {
        delete[] m_storage1;
        delete[] m_storage2;
        if (m_logFile != NULL)
            fclose(m_logFile);
    }

    /// Called by the host for each port to connect it to the looper.
    /// \param port The index of the port to be connected.
    /// \param data A pointer to the data where the parameter will be written to.
    void connectPort(PortIndex port, void* data)
    {
        // Install the trivial ports
        switch (port)
        {
            case LOOPER_INPUT1: m_input1 = (const float*)data; return;
            case LOOPER_INPUT2: m_input2 = (const float*)data; return;
            case LOOPER_OUTPUT1: m_output1 = (float*)data; return;
            case LOOPER_OUTPUT2: m_output2 = (float*)data; return;
            case LOOPER_THRESHOLD: m_thresholdParameter = (const float*)data; return;
            case LOOPER_DRY_AMOUNT: m_dryAmountParameter = (const float*)data; return;
            default: break;
        }

        // Install the buttons and set their callback functions.
        if (port == LOOPER_ACTIVATE)
        {
            m_activateButton.connect(data, [this](bool pressed, double interval, bool doubleClick)
            {
                if (!pressed)
                    return;
                if (doubleClick)
                {
                    reset();
                    return;
                }

                if (m_state == LOOPER_STATE_RECORDING || m_state == LOOPER_STATE_WAITING_FOR_THRESHOLD)
                    finishRecording();
                else
                    startRecording();
            });
        }
        else if (port == LOOPER_RESET)
        {
            m_resetButton.connect(data, [this](bool pressed, double interval, bool doubleClick)
            {
                if (!pressed)
                    return;
                if (doubleClick)
                {
                    reset();
                    return;
                }

                if (m_state == LOOPER_STATE_RECORDING || m_state == LOOPER_STATE_WAITING_FOR_THRESHOLD)
                    finishRecording();
                else
                    undo();
            });
        }
        else if (port == LOOPER_UNDO)
        {
            m_undoButton.connect(data, [this](bool pressed, double interval, bool doubleClick)
            {
                if (!pressed)
                    return;
                undo();
            });
        }
        else if (port == LOOPER_REDO)
        {
            m_redoButton.connect(data, [this](bool pressed, double interval, bool doubleClick)
            {
                if (!pressed)
                    return;
                redo();
            });
        }
        else if (port == LOOPER_DUB)
        {
            m_dubButton.connect(data, [this](bool pressed, double interval, bool doubleClick)
            {
               if (!pressed)
                    return;
                if (doubleClick)
                {
                    reset();
                    return;
                }

                if (m_state == LOOPER_STATE_RECORDING || m_state == LOOPER_STATE_WAITING_FOR_THRESHOLD)
                    finishRecording();
                startRecording();
            });
        }
    }

    /// Run the looper. Called for a bunch of samples at a time. Parameters will not change within this
    /// call!
    /// \param The number of samples to be read from the input and writte to the output.
    void run(uint32_t nrOfSamples)
    {
        updateParameters();

        m_now += double(nrOfSamples) / m_sampleRate;
        if (m_state == LOOPER_STATE_INACTIVE)
        {
            for (uint32_t s = 0; s < nrOfSamples; ++s)
            {
                m_output1[s] = m_dryAmount * m_input1[s];
                m_output2[s] = m_dryAmount * m_input2[s];
            }
            return;
        }

        for (uint32_t s = 0; s < nrOfSamples; ++s)
        {
            // Use the live input
            float in1 = m_input1[s];
            float in2 = m_input2[s];

            // Check if we reached the threshold to start recording.
            if (m_state == LOOPER_STATE_WAITING_FOR_THRESHOLD && (fabs(in1) >= m_threshold || fabs(in2) >= m_threshold))
            {
                Dub& dub = m_dubs[m_nrOfDubs];
                dub.m_startIndex = m_currentLoopIndex;
                m_state = LOOPER_STATE_RECORDING;
            }

            // If we are recoding do the record.
            if (m_state == LOOPER_STATE_RECORDING)
            {
                m_storage1[m_nrOfUsedSamples] = in1;
                m_storage2[m_nrOfUsedSamples] = in2;
                m_nrOfUsedSamples++;
                Dub& dub = m_dubs[m_nrOfDubs];
                dub.m_length++;
            }

            // Playback all active dubs.
            float out1 = m_dryAmount * in1;
            float out2 = m_dryAmount * in2;
            for (size_t t = 0; t < m_nrOfDubs; t++)
            {
                Dub& dub = m_dubs[t];
                if (m_currentLoopIndex < dub.m_startIndex)
                    continue;
                if (m_currentLoopIndex >= dub.m_startIndex + dub.m_length)
                    continue;
                size_t index = dub.m_storageOffset + (m_currentLoopIndex - dub.m_startIndex);
                out1 += m_storage1[index];
                out2 += m_storage2[index];
            }

            // Store accumulated output.
            m_output1[s] = out1;
            m_output2[s] = out2;

            if (m_nrOfDubs > 0)
                // Only once we are actually playing anything the loop length is known.
                m_currentLoopIndex++;

            // At the end increment the loop index and check if we are at the end
            // of the loop. The first dub governs the length of the whole loop.
            // So if still recording when we reach the end of the loop, we stop
            // the recording! Note that if we don't have a dub, yet, then m_loopLength
            // is 0, so no extra check is needed.
            if (m_currentLoopIndex > m_loopLength || m_nrOfUsedSamples >= m_storageSize)
            {
                // Reached the end of the loop, either because we exhausted storage
                // or the end of the loop is there.
                m_currentLoopIndex = 0;

                if (m_state == LOOPER_STATE_RECORDING)
                {
                    // Stop the recording only, if we did not have the threshold, yet.
                    // That allows to start recording right at the start of the loop.
                    finishRecording();
                    if(m_nrOfDubs > 1)
                    {
                        // This is the second dub, meaning we're overdubbing so don't
                        // actually stop recording dubs until the user clicks the
                        // button again.
                        startRecording();
                    }
                }
            }
        }
    }

private:
    //
    // Input parameters
    //

    /// Threshold parameter
    const float* m_thresholdParameter = NULL;

    /// Dry amount parameter
    const float* m_dryAmountParameter = NULL;

    /// Activate button
    MomentaryButton m_activateButton;
    /// Reset button
    MomentaryButton m_resetButton;
    /// Undo button
    MomentaryButton m_undoButton;
    /// Redo button
    MomentaryButton m_redoButton;
    /// Dub button
    MomentaryButton m_dubButton;

    //
    // All audio inputs
    //

    /// Audio input 1
    const float* m_input1 = NULL;
    /// Audio input 2
    const float* m_input2 = NULL;

    //
    // All audio outputs
    //

    /// Audio output 1
    float* m_output1 = NULL;
    /// audio output 2
    float* m_output2 = NULL;

    //
    // Internal state
    //

    /// The stored sample rate
    uint32_t m_sampleRate = 48000;
    /// The current looper state
    State m_state = LOOPER_STATE_INACTIVE;
    /// The stored threshold as a linear value
    float m_threshold = 0.0f;
    /// The stored dry amount
    float m_dryAmount = 1.0f;
    /// Where are we with the first (main) loop. The first loop governs all the loops!
    size_t m_currentLoopIndex = 0;
    /// The lenght of the main loop
    size_t m_loopLength = 0;
    /// Current time, sample accurate used for buttons
    double m_now = 0;

    //
    // Storage memory for audio
    //

    /// Overall storage size for audio (number of floats per channel)
    size_t m_storageSize = 0;
    /// Number of samples already used
    size_t m_nrOfUsedSamples = 0;
    /// Storage for first channel
    float* m_storage1 = NULL;
    /// Storage for second channel
    float* m_storage2 = NULL;

    //
    // Store information about the dubs
    //

    /// The number of dubs currently active
    size_t m_nrOfDubs = 0;
    /// The number of dubs which we recorded and thus could be redone
    size_t m_maxUsedDubs = 0;
    /// The dubs
    Dub m_dubs[NR_OF_DUBS];

    /// If we want to log to a file, we can use this.
    FILE* m_logFile = NULL;

    /// Log function (printf-style)
    void log(const char *formatString, ...)
    {
        if (!LOG_ENABLED)
            return;
        if (m_logFile == NULL)
            return;

        char buffer[2048];
        va_list argumentList;
        va_start(argumentList, formatString);
        vsnprintf(&buffer[0], sizeof(buffer), formatString, argumentList);
        va_end(argumentList);
        fwrite(buffer, 1, strlen(buffer), m_logFile);
        fprintf(m_logFile, "\n");
        fflush(m_logFile);
    }

    /// Reset everything to initial state.
    void reset()
    {
        m_nrOfDubs = 0;
        m_maxUsedDubs = 0;
        m_nrOfUsedSamples = 0;
        m_state = LOOPER_STATE_INACTIVE;
        m_currentLoopIndex = 0;
        m_loopLength = 0;
        m_nrOfUsedSamples = 0;
    }

    /// Start recording a dub if possible (a dub and memory for audio left).
    void startRecording()
    {
        if (m_nrOfDubs >= NR_OF_DUBS)
            // Reached maximum number of dubs, cannot start recording.
            return;
        if (m_nrOfUsedSamples >= m_storageSize)
            // Memory full, cannot start recording.
            return;

        // Prepare the dub.
        Dub& dub = m_dubs[m_nrOfDubs];
        dub.m_storageOffset = m_nrOfUsedSamples;
        dub.m_length = 0;

        // Now start the recording.
        m_state = LOOPER_STATE_WAITING_FOR_THRESHOLD;
    }

    /// Finish the recording.
    void finishRecording()
    {
        if (m_state == LOOPER_STATE_WAITING_FOR_THRESHOLD)
        {
            // We did not actually record anything, yet. So nothing to do. Just
            // go back to the previous state.
            if (m_nrOfDubs == 0)
                m_state = LOOPER_STATE_INACTIVE;
            else
                m_state = LOOPER_STATE_PLAYING;
            return;
        }

        // We did record something, so make sure we will use it.
        m_state = LOOPER_STATE_PLAYING;
        Dub& dub = m_dubs[m_nrOfDubs];
        if (m_nrOfDubs == 0)
        {
            // This was the first dub which governs the loop length.
            m_loopLength = dub.m_length;
            m_currentLoopIndex = 0;
        }

        // Fixup the start and the end of the loop. We simply fade in and out over
        // 32 samples for now. Not sure if that's good for everything, seems to work
        // nicely enough, though.
        size_t length = dub.m_length > NR_OF_BLEND_SAMPLES ? NR_OF_BLEND_SAMPLES : dub.m_length;
        size_t startIndex = dub.m_storageOffset;
        size_t endIndex = dub.m_storageOffset + dub.m_length - 1;
        for (size_t s = 0; s < length; s++)
        {
            float factor = float(s) / length;
            m_storage1[startIndex] *= factor;
            m_storage2[startIndex] *= factor;
            startIndex++;
            m_storage1[endIndex] *= factor;
            m_storage2[endIndex] *= factor;
            endIndex--;
        }

        // Now the dub is officially ready for playing...
        m_nrOfDubs++;

        // Note that when recording a new dub we need to reset max dubs as well, even if
        // once had more dubs: They have been overwritten and cannot be redone!
        m_maxUsedDubs = m_nrOfDubs;
    }

    /// Undo the last recorded dub, if there is any. Will also stop recording. So a currently
    /// recording dub will not be heard but could be redone!
    void undo()
    {
        if (m_state == LOOPER_STATE_RECORDING)
            // When we are recording, we interpret undo as undoing the current recording.
            // So we simply finish it and then immediately undo.
            finishRecording();
        if (m_nrOfDubs == 0)
            // Nothing to undo.
            return;

        // Deactivate the undone dub.
        m_nrOfDubs--;
        Dub& dub = m_dubs[m_nrOfDubs];
        // Make sure that next time we record the undone dub will be overwritten. Recording
        // next time will invalidate any possiblity to redo!
        m_nrOfUsedSamples = dub.m_storageOffset;
        if (m_nrOfDubs == 0)
        {
            // When undoing the first dub, then we stop playing. It is like a reset, but
            // we can still redo.
            m_loopLength = 0;
            m_currentLoopIndex = 0;
        }
    }

    /// Redo a dub. Redo is possible as many times as an undo was done _after_ the last
    /// recording operation. Recording will invalidate all redos - similar to what a
    /// text editor does.
    void redo()
    {
        if (m_state == LOOPER_STATE_RECORDING)
            // Cannot redo if recording, redo info is overwritten.
            return;
        if (m_nrOfDubs == m_maxUsedDubs)
            // Nothing to redo here, we are already at the last track.
            return;

        // Can redo!
        Dub& dub = m_dubs[m_nrOfDubs];
        // Make sure that we do not overwrite the dubs audio data when recording
        // next time.
        m_nrOfUsedSamples = dub.m_storageOffset + dub.m_length;
        if (m_nrOfDubs == 0)
        {
            // If redoing the first dub, then we start playback from the beginning.
            m_currentLoopIndex = 0;
            m_loopLength = dub.m_length;
        }

        // Now activate the redone dub.
        m_nrOfDubs++;
    }

    /// Update all the parameters from the inputs.
    void updateParameters()
    {
        m_threshold = dbToFloat(*m_thresholdParameter);
        m_dryAmount = *m_dryAmountParameter;
        m_activateButton.run(m_now);
        m_resetButton.run(m_now);
        m_undoButton.run(m_now);
        m_redoButton.run(m_now);
        m_dubButton.run(m_now);
    }
};

//
// The functions required by the LV2 interface. Simply forward to the Looper class.
//
static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double rate, const char* bundlePath,
    const LV2_Feature* const* features)
{
    return (LV2_Handle)new Looper(rate);
}
static void activate(LV2_Handle instance) {}
static void deactivate(LV2_Handle instance) {}
static void cleanup(LV2_Handle instance) { delete static_cast<Looper*>(instance); }
static const void* extensionData(const char* uri) { return NULL; }
static void connectPort(LV2_Handle instance, uint32_t port, void* data)
{
    Looper* looper = static_cast<Looper*>(instance);
    looper->connectPort(static_cast<PortIndex>(port), data);
}
static void run(LV2_Handle instance, uint32_t nrOfSamples)
{
    Looper* looper = static_cast<Looper*>(instance);
    looper->run(nrOfSamples);
}

///
/// Descriptors for the various functions called by the LV2 host
///
static const LV2_Descriptor descriptor =
{
    /// The URI which identifies the plugin
    LOOPER_URI,
    /// Instantiate the plugin.
    instantiate,
    /// Connect a port, called once for each port.
    connectPort,
    /// Activate the plugin (unused).
    activate,
    /// Process a bunch of samples.
    run,
    /// Deactivate the plugin (unused).
    deactivate,
    /// Cleanup, will destroy the plugin.
    cleanup,
    /// Get information about used extensions (unused).
    extensionData
};

///
/// DLL entry point which is called with index 0.. until it returns NULL.
///
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    switch (index)
    {
        case 0:  return &descriptor;
        default: return NULL;
    }
}
