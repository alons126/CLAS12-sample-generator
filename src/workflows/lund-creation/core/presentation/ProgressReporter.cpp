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
 *   Maintained execution platforms provide the POSIX isatty() interface. A carriage return refreshes an
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
    if (!line_active_) { return; }

    try {
        std::cout << '\n';
    } catch (...) {
        // Progress cleanup must never replace the workflow result during stack unwinding.
    }
}
#pragma endregion

// Progress updates ------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma region /* Progress updates */
bool ProgressReporter::updateDue(Clock::time_point now) const {
    if (!printed_) { return true; }

    const auto interval = interactive_ ? std::chrono::milliseconds(100) : std::chrono::seconds(10);
    return now - last_print_ >= interval;
}

void ProgressReporter::update(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total) {
    if (finished_) { return; }

    const auto now = Clock::now();
    if (!updateDue(now)) { return; }

    render(completed, secondary_completed, secondary_total, {}, false);
    last_print_ = now;
}

void ProgressReporter::finish(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome) {
    if (finished_) { return; }

    render(completed, secondary_completed, secondary_total, outcome, true);
    finished_ = true;
}

void ProgressReporter::render(std::uint64_t completed, std::uint64_t secondary_completed, std::uint64_t secondary_total, const std::string& outcome, bool final) {
    constexpr std::uint64_t bar_width = 24;
    const auto bounded_completed = std::min(completed, total_);
    const auto percentage = static_cast<std::uint64_t>((static_cast<long double>(bounded_completed) * 100.0L) / static_cast<long double>(total_));
    const auto filled = percentage * bar_width / 100;

    std::ostringstream line;
    line << '[';

    // Color only the completed cells. Keep brackets, remaining cells, percentage, and counters unchanged.
    if (interactive_ && filled > 0) { line << env::COMPLETION_COLOR; }

    line << std::string(filled, '#');

    if (interactive_ && filled > 0) { line << env::RESET_COLOR; }

    line << std::string(bar_width - filled, '-') << "] " << std::setw(3) << percentage << '%';

    if (primary_label_.empty()) {
        line << ' ' << completed << '/' << total_;
    } else {
        line << " | " << primary_label_ << ": " << completed << '/' << total_;
    }

    if (!secondary_label_.empty() && secondary_total > 0) { line << " | " << secondary_label_ << ": " << secondary_completed << '/' << secondary_total; }
    if (!outcome.empty()) { line << " | " << outcome; }

    if (interactive_) {
        std::cout << '\r' << env::SYSTEM_COLOR << activity_ << env::RESET_COLOR << ' ' << line.str() << std::flush;
    } else {
        std::cout << activity_ << ": " << line.str() << '\n';
    }

    printed_ = true;
    line_active_ = interactive_ && !final;

    if (interactive_ && final) { std::cout << '\n'; }
}
#pragma endregion

}  // namespace samples
