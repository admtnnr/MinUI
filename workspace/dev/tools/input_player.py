#!/usr/bin/env python3
"""
MinUI Input Player
Replays recorded input sequences by simulating keypresses via X11
"""

import sys
import time
import csv
import argparse
from pathlib import Path

try:
    import Xlib.display
    import Xlib.X
    import Xlib.XK
    import Xlib.protocol.event
except ImportError:
    print("Error: python3-xlib is required")
    print("Install with: pip3 install python-xlib")
    sys.exit(1)


# Button to keycode mapping (matches platform.c)
BUTTON_KEY_MAP = {
    0x0001: 'Up',       # BTN_UP
    0x0002: 'Down',     # BTN_DOWN
    0x0004: 'Left',     # BTN_LEFT
    0x0008: 'Right',    # BTN_RIGHT
    0x0010: 'x',        # BTN_A
    0x0020: 'z',        # BTN_B
    0x0040: 's',        # BTN_X
    0x0080: 'a',        # BTN_Y
    0x0100: 'q',        # BTN_L1
    0x0200: 'w',        # BTN_R1
    0x0400: 'e',        # BTN_L2
    0x0800: 'r',        # BTN_R2
    0x1000: 't',        # BTN_L3
    0x2000: 'y',        # BTN_R3
    0x4000: 'Shift_R',  # BTN_SELECT
    0x8000: 'Return',   # BTN_START
    0x10000: 'Escape',  # BTN_MENU
    0x20000: 'p',       # BTN_POWER
    0x40000: 'equal',   # BTN_PLUS
    0x80000: 'minus',   # BTN_MINUS
}


class InputPlayer:
    def __init__(self, window_name="MinUI Dev"):
        self.display = Xlib.display.Display()
        self.window_name = window_name
        self.target_window = None
        self.pressed_keys = set()

    def find_window(self):
        """Find MinUI window by name"""
        root = self.display.screen().root

        def search_window(window):
            try:
                name = window.get_wm_name()
                if name and self.window_name in name:
                    return window

                for child in window.query_tree().children:
                    result = search_window(child)
                    if result:
                        return result
            except:
                pass
            return None

        self.target_window = search_window(root)
        if not self.target_window:
            print(f"Warning: Window '{self.window_name}' not found, using root window")
            self.target_window = root
        else:
            print(f"Found window: {self.target_window.get_wm_name()}")

        return self.target_window is not None

    def press_key(self, key_name):
        """Press a key"""
        keysym = Xlib.XK.string_to_keysym(key_name)
        if keysym == 0:
            print(f"Warning: Unknown key '{key_name}'")
            return

        keycode = self.display.keysym_to_keycode(keysym)
        if keycode == 0:
            print(f"Warning: No keycode for '{key_name}'")
            return

        # Send key press event
        event = Xlib.protocol.event.KeyPress(
            time=int(time.time()),
            root=self.display.screen().root,
            window=self.target_window,
            same_screen=0,
            child=Xlib.X.NONE,
            root_x=0,
            root_y=0,
            event_x=0,
            event_y=0,
            state=0,
            detail=keycode
        )
        self.target_window.send_event(event, propagate=True)
        self.pressed_keys.add(key_name)
        self.display.flush()

    def release_key(self, key_name):
        """Release a key"""
        keysym = Xlib.XK.string_to_keysym(key_name)
        if keysym == 0:
            return

        keycode = self.display.keysym_to_keycode(keysym)
        if keycode == 0:
            return

        # Send key release event
        event = Xlib.protocol.event.KeyRelease(
            time=int(time.time()),
            root=self.display.screen().root,
            window=self.target_window,
            same_screen=0,
            child=Xlib.X.NONE,
            root_x=0,
            root_y=0,
            event_x=0,
            event_y=0,
            state=0,
            detail=keycode
        )
        self.target_window.send_event(event, propagate=True)
        self.pressed_keys.discard(key_name)
        self.display.flush()

    def release_all_keys(self):
        """Release all currently pressed keys"""
        for key in list(self.pressed_keys):
            self.release_key(key)

    def set_button_state(self, button_state):
        """Set buttons based on state bitmask"""
        # Release keys that are no longer pressed
        for button_mask, key_name in BUTTON_KEY_MAP.items():
            if key_name in self.pressed_keys and not (button_state & button_mask):
                self.release_key(key_name)

        # Press keys that should be pressed
        for button_mask, key_name in BUTTON_KEY_MAP.items():
            if (button_state & button_mask) and key_name not in self.pressed_keys:
                self.press_key(key_name)

    def play_recording(self, recording_file, speed=1.0):
        """Play back a recorded input sequence"""
        if not self.find_window():
            print("Warning: Target window not found, playback may not work")

        print(f"Playing recording: {recording_file} (speed: {speed}x)")

        with open(recording_file, 'r') as f:
            reader = csv.reader(f)
            last_timestamp = None

            for row in reader:
                # Skip comments and headers
                if not row or row[0].startswith('#'):
                    continue

                try:
                    timestamp = int(row[0])
                    button_state = int(row[1])

                    # Calculate delay
                    if last_timestamp is not None:
                        delay = (timestamp - last_timestamp) / 1000.0 / speed
                        if delay > 0:
                            time.sleep(delay)

                    # Apply button state
                    self.set_button_state(button_state)

                    last_timestamp = timestamp

                except (ValueError, IndexError) as e:
                    print(f"Warning: Skipping invalid row: {row}")
                    continue

        # Release all keys at the end
        self.release_all_keys()
        print("Playback complete")

    def close(self):
        """Clean up"""
        self.release_all_keys()
        self.display.close()


def main():
    parser = argparse.ArgumentParser(description='MinUI Input Player - Replay recorded inputs')
    parser.add_argument('recording', type=str, help='Input recording file (CSV)')
    parser.add_argument('--speed', type=float, default=1.0, help='Playback speed multiplier (default: 1.0)')
    parser.add_argument('--window', type=str, default='MinUI Dev', help='Window name to target')
    parser.add_argument('--delay', type=float, default=1.0, help='Initial delay before playback (seconds)')

    args = parser.parse_args()

    recording_path = Path(args.recording)
    if not recording_path.exists():
        print(f"Error: Recording file not found: {recording_path}")
        return 1

    print(f"Starting playback in {args.delay} seconds...")
    time.sleep(args.delay)

    player = InputPlayer(window_name=args.window)

    try:
        player.play_recording(args.recording, speed=args.speed)
    except KeyboardInterrupt:
        print("\nPlayback interrupted")
    except Exception as e:
        print(f"Error during playback: {e}")
        import traceback
        traceback.print_exc()
    finally:
        player.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
