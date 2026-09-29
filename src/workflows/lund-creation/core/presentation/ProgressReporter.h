//
// Created by Alon Sportes on 27/09/2026.
//

/**
 * @file ProgressReporter.h
 * @brief Shows how many LUND events have been processed while a workflow is running.
 *
 * Purpose:
 *   Event creation may take a long time. ProgressReporter shows that work is continuing and how much has
 *   finished. For example, it can show:
 *      Generating LUND events [######------------------] 25% 250/1000
 *   The uniform LUND creator creates new random test events. The physical LUND converter reads existing
 *   events from an event-generator file and writes them as LUND. Both use this progress display.
 *
 * Usage:
 *   Create one reporter with a short activity name and the total amount of work. Call update() with the
 *   newest counts as events finish. Call finish() when the loop ends and before printing the next message.
 *   When output is shown directly in a terminal, one line changes in place. When output is saved to a file
 *   or a batch-job log, the reporter writes a new line only occasionally.
 *
 * Ownership and lifetime:
 *   The reporter stores its own copies of the labels and keeps track of when it last printed. It writes to
 *   standard output. It does not own or change the code doing the event work, the LUND writer, the
 *   configuration, or the workflow counters.
 *
 * Failure behavior:
 *   The constructor reports an error if the total is zero because a percentage cannot be calculated from
 *   zero. Destroying a reporter ends an unfinished terminal line, so a later warning or error begins on a
 *   clean line.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace samples {

// Progress display ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Progress display */
/**
 * @class ProgressReporter
 * @brief Displays progress for one stage without printing on every event.
 *
 * Output shown in a terminal:
 *   The reporter changes one line in place at most ten times per second. The bar and percentage use the
 *   main completed count and its total. Only the filled part of the bar uses COMPLETION_COLOR. A second
 *   counter may show related work, but it does not change the bar or percentage.
 *
 * Output saved to a log:
 *   When standard output goes to a file or batch-job log instead of a terminal, an old line cannot be
 *   replaced. The reporter therefore writes one line at the start, another line every ten seconds, and
 *   one final line. These lines contain no terminal colors.
 *
 * Invariants:
 *   total_ is greater than zero. If the completed count is larger than the total, the bar stops at 100%,
 *   but the exact count supplied by the caller is still printed. The first finish() call writes the final
 *   update and ends the terminal line. Later finish() calls do nothing.
 */
class ProgressReporter {
   public:
    /**
     * @brief Constructor: Create a progress display for one activity.
     * @param activity Text shown at the start of each update, such as `Generating LUND events`.
     * @param total Amount of work that represents 100%; must be greater than zero.
     * @param primary_label Optional name shown before the main completed count.
     * @param secondary_label Optional name for a second completed count that does not control the bar.
     * @throws std::invalid_argument If total is zero, because the percentage would be undefined.
     */
    ProgressReporter(std::string activity, std::uint64_t total, std::string primary_label = {}, std::string secondary_label = {});

    /**
     * @brief Destructor: End an unfinished terminal line when the reporter goes out of use.
     *
     * This cleanup does not change counters or mark the workflow as successful. It only makes sure the
     * next message starts on a new line.
     */
    ~ProgressReporter() noexcept;

    /** @brief Constructor: Copying is disabled because one reporter owns one displayed line and its print timing. */
    ProgressReporter(const ProgressReporter&) = delete;

    /** @brief Copy assignment is disabled because one reporter owns one displayed line and its timing. */
    ProgressReporter& operator=(const ProgressReporter&) = delete;

    /** @brief Constructor: Moving is disabled so cleanup remains tied to the object that started the displayed line. */
    ProgressReporter(ProgressReporter&&) = delete;

    /** @brief Move assignment is disabled so ownership of an active displayed line cannot change. */
    ProgressReporter& operator=(ProgressReporter&&) = delete;

    /**
     * @brief Show the newest counts if enough time has passed since the previous update.
     * @param completed Main completed count used for the percentage and bar.
     * @param secondary_completed Optional completed count for the second kind of work.
     * @param secondary_total Optional total for the second kind of work. Zero hides the second counter.
     *
     * Event loops may call this function after every event. The reporter skips calls that arrive too soon,
     * so frequent updates do not slow the work or fill a log with nearly identical lines.
     */
    void update(std::uint64_t completed, std::uint64_t secondary_completed = 0, std::uint64_t secondary_total = 0);

    /**
     * @brief Show the final counts immediately and end the progress display.
     * @param completed Final main completed count.
     * @param secondary_completed Optional final completed count for the second kind of work.
     * @param secondary_total Optional total for the second kind of work. Zero hides the second counter.
     * @param outcome Optional text explaining why the loop ended, such as `input exhausted`.
     *
     * Unlike update(), this function does not wait for the next print time. Only its first call has an
     * effect.
     */
    void finish(std::uint64_t completed, std::uint64_t secondary_completed = 0, std::uint64_t secondary_total = 0, const std::string& outcome = {});

    // Internal display helpers and state --------------------------------------------------------------------------------------------------------------------------------
   private:
    using Clock = std::chrono::steady_clock;  ///< Clock used to decide when another update may be printed.

    /**
     * @brief Check whether enough time has passed to print another normal update.
     * @param now Current time from Clock.
     * @return true for the first update, after 100 milliseconds in a terminal, or after 10 seconds when
     *         output is being saved to a log; otherwise false.
     */
    bool updateDue(Clock::time_point now) const;

    /**
     * @brief Build and print one progress update.
     * @param completed Main completed count.
     * @param secondary_completed Completed count for the optional second kind of work.
     * @param secondary_total Total for the optional second kind of work. Zero hides this counter.
     * @param outcome Optional reason the loop ended. Empty text hides it.
     * @param final true to end the progress display after this update; false to keep it active.
     */
    void render(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome, bool final);

    std::string activity_;          ///< Text shown at the start of every update.
    std::string primary_label_;     ///< Optional name shown before the main completed count.
    std::string secondary_label_;   ///< Optional name shown before the second completed count.
    std::uint64_t total_;           ///< Main count that represents 100% completion.
    bool interactive_;              ///< true when output is shown directly in a terminal.
    bool printed_ = false;          ///< true after the reporter has printed at least one update.
    bool line_active_ = false;      ///< true when the changing terminal line still needs a newline.
    bool finished_ = false;         ///< true after the reporter has printed its final update.
    Clock::time_point last_print_;  ///< Time when the most recent update was printed.
};
#pragma endregion

}  // namespace samples
