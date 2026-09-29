//
// Created by Alon Sportes on 27/09/2026.
//

/**
 * @file ProgressReporter.cpp
 * @brief Prints a changing progress line in a terminal or occasional progress lines in a saved log.
 *
 * Purpose:
 *   An event loop may call update() after every event, but printing that often would slow the work and
 *   flood a saved log. This file decides when to print and builds a short line showing the completed
 *   count, percentage, and optional related information.
 *
 * Execution flow:
 *   The constructor checks whether output is going directly to a terminal. update() prints immediately the
 *   first time, then waits at least 100 milliseconds between terminal updates or 10 seconds between saved
 *   log lines. render() builds the bar and counters. finish() prints the newest values immediately and
 *   ends the display. If the workflow stops before finish(), the destructor moves later output to a clean
 *   line.
 *
 * Scope:
 *   This file only displays counts supplied by the event-producing code. It does not count events, decide
 *   when an event loop should stop, write LUND files, or decide whether the workflow succeeded.
 *
 * Assumptions:
 *   The system provides isatty(), which reports whether standard output is connected directly to a
 *   terminal. In a terminal, the `\r` character moves the cursor back to the start of the current line so
 *   the next update can replace it. Saved logs receive normal lines instead.
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

// Starting and ending the display ---------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Starting and ending the display */
// isatty() returns true when standard output is shown directly in a terminal. That choice stays fixed for
// the life of this reporter.
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
    // While work is running, a terminal bar has no newline because its next update must replace the same
    // line. There is nothing to do if no such line exists or finish() already ended it.
    if (!line_active_) { return; }

    // If the workflow stops early, add the missing newline so the next warning or error appears below the
    // bar instead of at its end.
    try {
        std::cout << '\n';
    } catch (...) {
        // Another error may already be stopping the workflow. Do not replace that useful error with a
        // second error caused only by trying to write this cleanup newline.
    }
}
#pragma endregion

// Choosing and printing updates -----------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Choosing and printing updates */
bool ProgressReporter::updateDue(Clock::time_point now) const {
    // Print the first values immediately so the user can see that work has started.
    if (!printed_) { return true; }

    // A terminal can replace one line frequently without adding clutter. A saved log cannot replace text,
    // so wait ten seconds between its lines to keep the file readable.
    const auto interval = interactive_ ? std::chrono::milliseconds(100) : std::chrono::seconds(10);

    // Print only when the selected wait time has passed since the previous line.
    return now - last_print_ >= interval;
}

void ProgressReporter::update(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total) {
    // finish() has already printed the final values, so later updates must not change them.
    if (finished_) { return; }

    // The event loop may call update() after every event. Skip this call when the display changed too
    // recently.
    const auto now = Clock::now();
    if (!updateDue(now)) { return; }

    // Print the newest values and remember the time. Skipped calls do not need to be saved because the
    // next allowed call supplies the newest totals again.
    render(completed, secondary_completed, secondary_total, {}, false);
    last_print_ = now;
}

void ProgressReporter::finish(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome) {
    // Print the final line only once, even if more than one cleanup path calls finish().
    if (finished_) { return; }

    // The event loop has ended, so show its newest counts and stop reason now instead of waiting for the
    // normal update time.
    render(completed, secondary_completed, secondary_total, outcome, true);

    // Keep later update() or finish() calls from changing the completed display.
    finished_ = true;
}

void ProgressReporter::render(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome, bool final) {
    // Every bar contains 24 positions, so its displayed width stays fixed as work progresses.
    constexpr std::uint64_t bar_width = 24;

    // Stop the bar and percentage at 100% if the supplied completed count is too large. The numeric count
    // printed later still shows the exact value supplied by the caller.
    const auto bounded_completed = std::min(completed, total_);

    // Convert completed/total to a whole percentage. Use that percentage to fill the matching number of
    // bar positions with `#`; the unfilled positions later receive `-`.
    const auto percentage = static_cast<std::uint64_t>((static_cast<long double>(bounded_completed) * 100.0L) / static_cast<long double>(total_));
    const auto filled = percentage * bar_width / 100;

    // Build the complete bar and counters in memory before writing them as one update.
    std::ostringstream line;
    line << '[';

    // In a terminal, color only the completed `#` positions. Saved logs contain no color instructions.
    if (interactive_ && (filled > 0)) { line << env::COMPLETION_COLOR; }

    line << std::string(filled, '#');

    if (interactive_ && (filled > 0)) { line << env::RESET_COLOR; }

    // Add the unfilled part of the bar. Reserve three spaces for the percentage number so 0%, 10%, and
    // 100% all end in the same column.
    line << std::string(bar_width - filled, '-') << "] " << std::setw(3) << percentage << '%';

    // Show a named main counter as `| events: 25/100`. Without a label, show the shorter `25/100`.
    if (primary_label_.empty()) {
        line << ' ' << completed << '/' << total_;
    } else {
        line << " | " << primary_label_ << ": " << completed << '/' << total_;
    }

    // Show the second count only when it has both a label and a total. Show the stop reason only when the
    // caller supplies one.
    if (!secondary_label_.empty() && (secondary_total > 0)) { line << " | " << secondary_label_ << ": " << secondary_completed << '/' << secondary_total; }
    if (!outcome.empty()) { line << " | " << outcome; }

    // In a terminal, `\r` moves the cursor to the start of the current line, and flush sends the new text
    // to the screen immediately. A saved log cannot replace a line, so give each update its own newline.
    if (interactive_) {
        std::cout << '\r' << activity_ << ' ' << line.str() << std::flush;
    } else {
        std::cout << activity_ << ": " << line.str() << '\n';
    }

    // Record that output now exists. A non-final terminal update still needs a newline when the display
    // finishes or the object is destroyed.
    printed_ = true;
    line_active_ = interactive_ && !final;

    // After the final terminal update, add the newline that places the next stage message below the bar.
    if (interactive_ && final) { std::cout << '\n'; }
}
#pragma endregion

}  // namespace samples
