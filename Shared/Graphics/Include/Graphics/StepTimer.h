//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

namespace Graphics
{
    // Helper class for animation and simulation timing.
    class StepTimer
    {
    public:
        StepTimer() :
            _elapsedTicks(0),
            _totalTicks(0),
            _leftOverTicks(0),
            _frameCount(0),
            _framesPerSecond(0),
            _framesThisSecond(0),
            _qpcSecondCounter(0),
            _isFixedTimeStep(false),
            _targetElapsedTicks(TicksPerSecond / 60)
        {
            _qpcFrequency = GetPerformanceFrequency();

            // Initialize max delta to 1/10 of a second.
            _qpcMaxDelta = _qpcFrequency / 10;
        }

        // Get elapsed time since the previous Update call.
        uint64 GetElapsedTicks() const                        { return _elapsedTicks;                  }
        double GetElapsedSeconds() const                      { return TicksToSeconds(_elapsedTicks);  }

        // Get total time since the start of the program.
        uint64 GetTotalTicks() const                          { return _totalTicks;                    }
        double GetTotalSeconds() const                        { return TicksToSeconds(_totalTicks);    }

        // Get total number of updates since start of the program.
        uint32 GetFrameCount() const                          { return _frameCount;                    }

        // Get the current framerate.
        uint32 GetFramesPerSecond() const                     { return _framesPerSecond;               }

        // Set whether to use fixed or variable timestep mode.
        void SetFixedTimeStep(bool isFixedTimestep)           { _isFixedTimeStep = isFixedTimestep;    }

        // Set how often to call Update when in fixed timestep mode.
        void SetTargetElapsedTicks(uint64 targetElapsed)      { _targetElapsedTicks = targetElapsed;   }
        void SetTargetElapsedSeconds(double targetElapsed)    { _targetElapsedTicks = SecondsToTicks(targetElapsed);   }

        // Integer format represents time using 10,000,000 ticks per second.
        static const uint64 TicksPerSecond = 10'000'000;

        static double TicksToSeconds(uint64 ticks)            { return static_cast<double>(ticks) / TicksPerSecond;     }
        static uint64 SecondsToTicks(double seconds)          { return static_cast<uint64>(seconds * TicksPerSecond);   }

        // Convenient wrapper for QueryPerformanceFrequency. Throws an exception if
        // the call to QueryPerformanceFrequency fails.
        static inline uint64 GetPerformanceFrequency()
        {
            LARGE_INTEGER freq;
            if (!QueryPerformanceFrequency(&freq))
            {
                throw ref new Platform::FailureException();
            }
            return freq.QuadPart;
        }

        // Gets the current number of ticks from QueryPerformanceCounter. Throws an
        // exception if the call to QueryPerformanceCounter fails.
        static inline int64 GetTicks()
        {
            LARGE_INTEGER ticks;
            if (!QueryPerformanceCounter(&ticks))
            {
                throw ref new Platform::FailureException();
            }
            return ticks.QuadPart;
        }

        // After an intentional timing discontinuity (for instance a blocking IO operation)
        // call this to avoid having the fixed timestep logic attempt a set of catch-up
        // Update calls.

        void ResetElapsedTime()
        {
            _qpcLastTime = GetTicks();

            _leftOverTicks    = 0;
            _framesPerSecond  = 0;
            _framesThisSecond = 0;
            _qpcSecondCounter = 0;
        }

        // Update timer state, calling the specified Update function the appropriate number of times.
        template<typename TUpdate>
        void Tick(const TUpdate& update)
        {
            // Query the current time.
            uint64 currentTime = GetTicks();
            uint64 timeDelta   = currentTime - _qpcLastTime;

            _qpcLastTime      = currentTime;
            _qpcSecondCounter += timeDelta;

            // Clamp excessively large time deltas (e.g. after paused in the debugger).
            if (timeDelta > _qpcMaxDelta)
            {
                timeDelta = _qpcMaxDelta;
            }

            // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
            timeDelta *= TicksPerSecond;
            timeDelta /= _qpcFrequency;

            uint32 lastFrameCount = _frameCount;

            if (_isFixedTimeStep)
            {
                // Fixed timestep update logic

                // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
                // the clock to exactly match the target value. This prevents tiny and irrelevant errors
                // from accumulating over time. Without this clamping, a game that requested a 60 fps
                // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
                // accumulate enough tiny errors that it would drop a frame. It is better to just round
                // small deviations down to zero to leave things running smoothly.

                if (abs(static_cast<int64>(timeDelta - _targetElapsedTicks)) < TicksPerSecond / 4000)
                {
                    timeDelta = _targetElapsedTicks;
                }

                _leftOverTicks += timeDelta;

                while (_leftOverTicks >= _targetElapsedTicks)
                {
                    _elapsedTicks   = _targetElapsedTicks;
                    _totalTicks    += _targetElapsedTicks;
                    _leftOverTicks -= _targetElapsedTicks;
                    _frameCount++;

                    update();
                }
            }
            else
            {
                // Variable timestep update logic.
                _elapsedTicks  = timeDelta;
                _totalTicks   += timeDelta;
                _leftOverTicks = 0;
                _frameCount++;

                update();
            }

            // Track the current framerate.
            if (_frameCount != lastFrameCount)
            {
                _framesThisSecond++;
            }

            if (_qpcSecondCounter >= static_cast<uint64>(_qpcFrequency))
            {
                _framesPerSecond   = _framesThisSecond;
                _framesThisSecond  = 0;
                _qpcSecondCounter %= _qpcFrequency;
            }
        }

    private:

        // Source timing data uses QPC units.
        uint64 _qpcFrequency;
        uint64 _qpcLastTime;
        uint64 _qpcMaxDelta;

        // Derived timing data uses a canonical tick format.
        uint64 _elapsedTicks;
        uint64 _totalTicks;
        uint64 _leftOverTicks;

        // Members for tracking the framerate.
        uint32 _frameCount;
        uint32 _framesPerSecond;
        uint32 _framesThisSecond;
        uint64 _qpcSecondCounter;

        // Members for configuring fixed timestep mode.
        bool   _isFixedTimeStep;
        uint64 _targetElapsedTicks;
    };
}
