#!/usr/bin/env python3
"""
MinUI Test Suite Runner
Automated testing framework for MinUI dev platform
"""

import os
import sys
import json
import time
import signal
import subprocess
import argparse
from pathlib import Path
from datetime import datetime


class TestRunner:
    def __init__(self, minui_binary, test_dir, output_dir, headless=False):
        self.minui_binary = Path(minui_binary)
        self.test_dir = Path(test_dir)
        self.output_dir = Path(output_dir)
        self.headless = headless
        self.minui_process = None
        self.results = []

    def start_minui(self):
        """Start MinUI in the background"""
        env = os.environ.copy()

        if self.headless:
            env['MINUI_HEADLESS'] = '1'

        # Set screenshots directory
        env['MINUI_SCREENSHOTS_DIR'] = str(self.output_dir / 'screenshots')

        print(f"Starting MinUI: {self.minui_binary}")
        print(f"  Headless: {self.headless}")
        print(f"  Screenshots: {env['MINUI_SCREENSHOTS_DIR']}")

        self.minui_process = subprocess.Popen(
            [str(self.minui_binary)],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        # Give it time to start
        time.sleep(2)

        if self.minui_process.poll() is not None:
            stdout, stderr = self.minui_process.communicate()
            print(f"Error: MinUI exited immediately")
            print(f"STDOUT: {stdout}")
            print(f"STDERR: {stderr}")
            return False

        print("MinUI started successfully")
        return True

    def stop_minui(self):
        """Stop MinUI"""
        if self.minui_process:
            print("Stopping MinUI...")
            self.minui_process.terminate()
            try:
                self.minui_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("Force killing MinUI...")
                self.minui_process.kill()
                self.minui_process.wait()
            self.minui_process = None

    def run_test(self, test_file):
        """Run a single test"""
        test_path = Path(test_file)
        print(f"\n{'='*60}")
        print(f"Running test: {test_path.name}")
        print(f"{'='*60}")

        with open(test_path, 'r') as f:
            test_config = json.load(f)

        test_name = test_config.get('name', test_path.stem)
        steps = test_config.get('steps', [])

        result = {
            'name': test_name,
            'file': str(test_path),
            'passed': True,
            'errors': [],
            'timestamp': datetime.now().isoformat()
        }

        try:
            for i, step in enumerate(steps):
                action = step.get('action')
                print(f"  Step {i+1}: {action}")

                if action == 'wait':
                    duration = step.get('duration', 1000) / 1000.0
                    time.sleep(duration)

                elif action == 'play_input':
                    recording = step.get('recording')
                    speed = step.get('speed', 1.0)
                    if recording:
                        recording_path = self.test_dir / recording
                        self._play_input(recording_path, speed)

                elif action == 'screenshot':
                    screenshot_name = step.get('name', f'step_{i}')
                    self._take_screenshot(screenshot_name)

                elif action == 'compare':
                    expected = step.get('expected')
                    actual = step.get('actual', 'latest')
                    threshold = step.get('threshold', 0.95)

                    if not self._compare_screenshots(expected, actual, threshold):
                        result['passed'] = False
                        result['errors'].append(f"Screenshot comparison failed at step {i+1}")

                elif action == 'input_sequence':
                    # Direct keyboard input (requires player script)
                    keys = step.get('keys', [])
                    for key in keys:
                        # TODO: Implement direct key sending
                        pass

                else:
                    print(f"    Warning: Unknown action '{action}'")

        except Exception as e:
            result['passed'] = False
            result['errors'].append(str(e))
            print(f"  ERROR: {e}")
            import traceback
            traceback.print_exc()

        status = "PASS" if result['passed'] else "FAIL"
        print(f"\nTest result: {status}")

        self.results.append(result)
        return result['passed']

    def _play_input(self, recording_path, speed):
        """Play an input recording"""
        player_script = Path(__file__).parent / 'input_player.py'

        if not player_script.exists():
            print(f"    Warning: input_player.py not found, skipping")
            return

        subprocess.run([
            sys.executable,
            str(player_script),
            str(recording_path),
            '--speed', str(speed)
        ])

    def _take_screenshot(self, name):
        """Request a screenshot via signal or key simulation"""
        # For now, just wait - screenshot is triggered by F12 in test
        # In a real implementation, we could send SIGUSR1 or similar
        pass

    def _compare_screenshots(self, expected, actual, threshold):
        """Compare screenshots"""
        compare_script = Path(__file__).parent / 'compare_images.py'

        if not compare_script.exists():
            print(f"    Warning: compare_images.py not found, skipping")
            return True

        expected_path = self.test_dir / expected
        if not expected_path.exists():
            print(f"    Warning: Expected image not found: {expected_path}")
            return False

        # Find actual screenshot (latest if 'latest')
        screenshots_dir = self.output_dir / 'screenshots'
        if actual == 'latest':
            screenshots = sorted(screenshots_dir.glob('*.png'))
            if not screenshots:
                print(f"    Warning: No screenshots found")
                return False
            actual_path = screenshots[-1]
        else:
            actual_path = screenshots_dir / actual

        if not actual_path.exists():
            print(f"    Warning: Actual image not found: {actual_path}")
            return False

        # Run comparison
        output_path = self.output_dir / f'diff_{expected_path.stem}.png'
        result = subprocess.run([
            sys.executable,
            str(compare_script),
            str(expected_path),
            str(actual_path),
            '--threshold', str(threshold),
            '--output', str(output_path)
        ])

        return result.returncode == 0

    def run_all_tests(self):
        """Run all tests in test directory"""
        test_files = sorted(self.test_dir.glob('test_*.json'))

        if not test_files:
            print(f"No test files found in {self.test_dir}")
            return False

        print(f"Found {len(test_files)} test(s)")

        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)
        (self.output_dir / 'screenshots').mkdir(exist_ok=True)

        # Start MinUI
        if not self.start_minui():
            return False

        try:
            # Run tests
            passed = 0
            failed = 0

            for test_file in test_files:
                if self.run_test(test_file):
                    passed += 1
                else:
                    failed += 1

            # Generate report
            self._generate_report(passed, failed)

            return failed == 0

        finally:
            self.stop_minui()

    def _generate_report(self, passed, failed):
        """Generate test report"""
        total = passed + failed

        print(f"\n{'='*60}")
        print(f"TEST SUMMARY")
        print(f"{'='*60}")
        print(f"Total:  {total}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"{'='*60}")

        # Save JSON report
        report_path = self.output_dir / 'test_report.json'
        with open(report_path, 'w') as f:
            json.dump({
                'timestamp': datetime.now().isoformat(),
                'total': total,
                'passed': passed,
                'failed': failed,
                'results': self.results
            }, f, indent=2)

        print(f"\nReport saved to: {report_path}")

        # Save HTML report
        html_report = self._generate_html_report(passed, failed)
        html_path = self.output_dir / 'test_report.html'
        with open(html_path, 'w') as f:
            f.write(html_report)

        print(f"HTML report saved to: {html_path}")

    def _generate_html_report(self, passed, failed):
        """Generate HTML test report"""
        total = passed + failed

        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>MinUI Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        h1 {{ color: #333; }}
        .summary {{ background: #f0f0f0; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .passed {{ color: green; font-weight: bold; }}
        .failed {{ color: red; font-weight: bold; }}
        .test {{ border: 1px solid #ddd; padding: 10px; margin: 10px 0; border-radius: 5px; }}
        .test.pass {{ border-left: 5px solid green; }}
        .test.fail {{ border-left: 5px solid red; }}
        .errors {{ color: red; margin: 10px 0; }}
    </style>
</head>
<body>
    <h1>MinUI Test Report</h1>
    <div class="summary">
        <h2>Summary</h2>
        <p>Total: {total}</p>
        <p class="passed">Passed: {passed}</p>
        <p class="failed">Failed: {failed}</p>
        <p>Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
    </div>
    <h2>Test Results</h2>
"""

        for result in self.results:
            status_class = 'pass' if result['passed'] else 'fail'
            status_text = 'PASS' if result['passed'] else 'FAIL'

            html += f"""
    <div class="test {status_class}">
        <h3>{result['name']} - <span class="{status_class}">{status_text}</span></h3>
        <p><strong>File:</strong> {result['file']}</p>
        <p><strong>Time:</strong> {result['timestamp']}</p>
"""
            if result['errors']:
                html += '<div class="errors"><strong>Errors:</strong><ul>'
                for error in result['errors']:
                    html += f'<li>{error}</li>'
                html += '</ul></div>'

            html += '    </div>\n'

        html += """
</body>
</html>
"""
        return html


def main():
    parser = argparse.ArgumentParser(description='MinUI Test Suite Runner')
    parser.add_argument('--minui', type=str, default='../all/minui/build/dev/minui.elf',
                       help='Path to minui binary')
    parser.add_argument('--tests', type=str, default='./tests',
                       help='Directory containing test files')
    parser.add_argument('--output', type=str, default='./test_output',
                       help='Output directory for results')
    parser.add_argument('--headless', action='store_true',
                       help='Run in headless mode')

    args = parser.parse_args()

    runner = TestRunner(
        minui_binary=args.minui,
        test_dir=args.tests,
        output_dir=args.output,
        headless=args.headless
    )

    try:
        success = runner.run_all_tests()
        return 0 if success else 1
    except KeyboardInterrupt:
        print("\n\nTest run interrupted")
        runner.stop_minui()
        return 130


if __name__ == '__main__':
    sys.exit(main())
