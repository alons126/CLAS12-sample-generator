//
// Created by Alon Sportes on 27/09/2026.
//

/**
 * @file ProgressReporter.h
 * @brief Declares shared progress output for LUND creation event loops.
 *
 * Purpose:
 *   Give uniform generation and physical conversion one terminal-aware progress display without
 *   coupling their different event semantics.
 *
 * Usage:
 *   Construct with an activity, known primary total, and optional counter labels -> call update() as
 *   work succeeds -> call finish() before the next stage message. Interactive terminals receive one
 *   refreshed line; redirected batch logs receive occasional complete lines.
 *
 * Ownership and lifetime:
 *   The reporter owns its labels and timing state. It writes to standard output and stores no reference
 *   to a creator, converter, writer, or configuration object.
 *
 * Failure behavior:
 *   Construction rejects a zero primary total. Progress output does not change workflow counters or
 *   completion state. Destruction ends an active interactive line so a later error starts cleanly.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace samples {

// ProgressReporter object -----------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ProgressReporter object */
/**
 * @class ProgressReporter
 * @brief Render throttled progress for one finite LUND-creation stage.
 *
 * Interactive behavior:
 *   Replace one terminal line at most ten times per second. The percentage and bar describe the primary
 *   counter. An optional secondary counter reports related work without pretending it has the same
 *   denominator.
 *
 * Batch behavior:
 *   Write an initial line, periodic lines at ten-second intervals, and one final line. No carriage-return
 *   animation or terminal color is written to redirected output.
 *
 * Invariants:
 *   total_ is positive. completed values are clamped only for percentage and bar calculation; the printed
 *   counter keeps the caller's exact value. finish() ends any interactive line and may be called once.
 */
class ProgressReporter {
   public:
    /**
     * @brief Start a progress display for one activity.
     * @param activity Short present-participle label, such as `Generating LUND events`.
     * @param total Known denominator for the primary percentage; must be positive.
     * @param primary_label Optional word printed after the percentage, such as `scanned`.
     * @param secondary_label Optional label for a second completed/total counter.
     * @throws std::invalid_argument If total is zero.
     */
    ProgressReporter(std::string activity, std::uint64_t total, std::string primary_label = {}, std::string secondary_label = {});

    /** @brief End an unfinished interactive line without changing workflow state. */
    ~ProgressReporter() noexcept;

    ProgressReporter(const ProgressReporter&) = delete;
    ProgressReporter& operator=(const ProgressReporter&) = delete;
    ProgressReporter(ProgressReporter&&) = delete;
    ProgressReporter& operator=(ProgressReporter&&) = delete;

    /**
     * @brief Offer the latest counters to the throttled display.
     * @param completed Primary completed count used for the percentage and bar.
     * @param secondary_completed Optional completed value for the secondary counter.
     * @param secondary_total Optional denominator for the secondary counter. A zero value hides it.
     */
    void update(std::uint64_t completed, std::uint64_t secondary_completed = 0, std::uint64_t secondary_total = 0);

    /**
     * @brief Print the final counters and end the progress line.
     * @param completed Final primary completed count.
     * @param secondary_completed Final optional secondary completed count.
     * @param secondary_total Final optional secondary denominator. A zero value hides it.
     * @param outcome Optional reason the loop ended, such as `input exhausted`.
     */
    void finish(std::uint64_t completed, std::uint64_t secondary_completed = 0, std::uint64_t secondary_total = 0, const std::string& outcome = {});

   private:
    using Clock = std::chrono::steady_clock;

    /** @brief Decide whether a non-final update is due for the active output mode. */
    bool updateDue(Clock::time_point now) const;

    /** @brief Format and write one interactive refresh or complete batch-log line. */
    void render(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome, bool final);

    std::string activity_;          ///< Stable text at the beginning of every progress update.
    std::string primary_label_;     ///< Optional word explaining the percentage counter.
    std::string secondary_label_;   ///< Optional name for the related second counter.
    std::uint64_t total_;           ///< Positive denominator used for percentage and bar width.
    bool interactive_;              ///< True when standard output is attached to a terminal.
    bool printed_ = false;          ///< True after at least one update has been emitted.
    bool line_active_ = false;      ///< True while an interactive line still needs its ending newline.
    bool finished_ = false;         ///< True after finish() has emitted the final update.
    Clock::time_point last_print_;  ///< Time of the most recent emitted update.
};
#pragma endregion

}  // namespace samples
