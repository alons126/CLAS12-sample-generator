//
// Created by Alon Sportes on 27/09/2026.
//

/**
 * @file ProgressReporter.cpp
 * @brief Implements terminal and batch-log progress reporting for LUND creation.
 *
 * Workflow:
 *   Detect whether standard output is a terminal -> throttle frequent event-loop updates -> calculate a
 *   bounded percentage -> refresh one terminal line or append a complete batch line -> end the line on
 *   normal completion or stack unwinding.
 *
 * Implementation boundaries:
 *   This file owns presentation and timing only. Creators and converters retain all scientific counters,
 *   stop conditions, output files, and success or failure decisions.
 *
 * Assumptions:
 *   Supported execution platforms provide the POSIX isatty() interface. A carriage return refreshes an
 *   interactive line; no locally defined ANSI cursor-control sequence is needed.
 */

#include "core/presentation/ProgressReporter.h"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "support/environment.h"

namespace env = environment;

namespace samples {

// ProgressReporter lifecycle --------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* ProgressReporter lifecycle */
ProgressReporter::ProgressReporter(std::string activity, std::uint64_t total, std::string primary_label, std::string secondary_label)
    : activity_(std::move(activity)),
      primary_label_(std::move(primary_label)),
      secondary_label_(std::move(secondary_label)),
      total_(total),
      interactive_(::isatty(STDOUT_FILENO) != 0),
      last_print_(Clock::now()) {
    if (total_ == 0) { throw std::invalid_argument("Progress total must be positive"); }
}

ProgressReporter::~ProgressReporter() noexcept {
    // A dynamic progress bar stays on the current terminal line while work is running. If finish()
    // already ended that line, or no dynamic line was printed, there is nothing left to clean up.
    if (!line_active_) { return; }

    // End an unfinished progress line so the next warning or error starts on a new line instead of
    // appearing after the bar.
    try {
        std::cout << '\n';
    } catch (...) {
        // This destructor may run because another operation already failed. Ignore a console-write
        // failure here so it does not hide the original error that stopped the workflow.
    }
}
#pragma endregion

// Progress updates ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Progress updates */
bool ProgressReporter::updateDue(Clock::time_point now) const {
    // Show the first update immediately so the user can see that work has started.
    if (!printed_) { return true; }

    // A live terminal can refresh one line often without adding more lines. Redirected output cannot
    // replace a line, so update it only every ten seconds to keep batch log files readable.
    const auto interval = interactive_ ? std::chrono::milliseconds(100) : std::chrono::seconds(10);

    // Print again only after the selected amount of time has passed since the previous update.
    return now - last_print_ >= interval;
}

void ProgressReporter::update(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total) {
    // finish() prints the last update. Ignore later calls so nothing can overwrite that final line.
    if (finished_) { return; }

    // Check the current time before printing. Event loops may call update() for every event, but the
    // terminal or log should change only at the interval selected by updateDue().
    const auto now = Clock::now();
    if (!updateDue(now)) { return; }

    // Render the newest counts, then remember when they were printed. Counts from skipped calls are not
    // lost because the next allowed update receives and prints the caller's latest values.
    render(completed, secondary_completed, secondary_total, {}, false);
    last_print_ = now;
}

void ProgressReporter::finish(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome) {
    // The final progress line should appear only once, even if cleanup code calls finish() again.
    if (finished_) { return; }

    // Always print the newest counts and stop reason. Unlike update(), this does not wait for the normal
    // refresh interval because the event loop has ended.
    render(completed, secondary_completed, secondary_total, outcome, true);

    // Prevent any later update() or finish() call from changing the completed line.
    finished_ = true;
}

void ProgressReporter::render(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome, bool final) {
    // Draw every bar with 24 character cells so its width does not change as the count grows.
    constexpr std::uint64_t bar_width = 24;

    // Never draw more than a full bar. The printed counter still keeps the exact completed value, but
    // an accidental value above the total cannot produce a percentage above 100 or a negative gap.
    const auto bounded_completed = std::min(completed, total_);

    // Convert the completed fraction to a whole percentage, then use that percentage to decide how many
    // of the 24 cells contain `#`. The remaining cells contain `-`.
    const auto percentage = static_cast<std::uint64_t>((static_cast<long double>(bounded_completed) * 100.0L) / static_cast<long double>(total_));
    const auto filled = percentage * bar_width / 100;

    // Build the entire changing part of the display before writing it to the terminal or log.
    std::ostringstream line;
    line << '[';

    // Color only the completed cells. Keep brackets, remaining cells, percentage, and counters unchanged.
    if (interactive_ && filled > 0) { line << env::COMPLETION_COLOR; }

    line << std::string(filled, '#');

    if (interactive_ && filled > 0) { line << env::RESET_COLOR; }

    // Complete the bar and print the percentage in a fixed three-character field so 0%, 10%, and 100%
    // line up in the same column.
    line << std::string(bar_width - filled, '-') << "] " << std::setw(3) << percentage << '%';

    // A named primary counter is separated from the bar with `|`. An unnamed counter follows the
    // percentage directly, which keeps the uniform-creator display compact.
    if (primary_label_.empty()) {
        line << ' ' << completed << '/' << total_;
    } else {
        line << " | " << primary_label_ << ": " << completed << '/' << total_;
    }

    // Add the second counter only when it has both a label and a useful total. Add the outcome only to
    // calls that provide one, normally the final physical-conversion update.
    if (!secondary_label_.empty() && secondary_total > 0) { line << " | " << secondary_label_ << ": " << secondary_completed << '/' << secondary_total; }
    if (!outcome.empty()) { line << " | " << outcome; }

    // A carriage return moves a live terminal back to the start of its current line, letting this update
    // replace the previous bar. A log cannot replace text, so each update becomes a normal new line.
    if (interactive_) {
        std::cout << '\r' << activity_ << ' ' << line.str() << std::flush;
        // std::cout << '\r' << env::SYSTEM_COLOR << activity_ << env::RESET_COLOR << ' ' << line.str() << std::flush;
    } else {
        std::cout << activity_ << ": " << line.str() << '\n';
    }

    // Remember that output exists and whether the terminal line is still waiting for a newline.
    printed_ = true;
    line_active_ = interactive_ && !final;

    // End the dynamic line after the final update so the next stage message starts below the bar.
    if (interactive_ && final) { std::cout << '\n'; }
}
#pragma endregion

}  // namespace samples
