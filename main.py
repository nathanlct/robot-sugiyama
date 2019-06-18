import pynput
from pynput.keyboard import Key
import time



# whether to automatically switch to Makeblock using Cmd + Tab
# or wait for the next click to open the Makeblock window
SWITCH_AUTOMATICALLY = False

KEYS = {
    'accelerate': 'a'
    'decelerate': 
}






def main():
    keyboard = pynput.keyboard.Controller()

    for i in range(4):
        keyboard.press('a')
        keyboard.release('a')
        time.sleep(0.5)
    keyboard.press(Key.space)
    keyboard.release(Key.space)

    # def get_distance_from_policy()

    # while True:
    #     distance = get_heading_distance_from_camera()



if __name__ == '__main__':
    if SWITCH_AUTOMATICALLY:
        keyboard.press(Key.cmd)
        keyboard.press(Key.tab)
        keyboard.release(Key.tab)
        keyboard.release(Key.cmd)

    else:
        def on_click(x, y, button, pressed):
            if not pressed:  # click is released
                return False  # stop listening

        mouse = pynput.mouse.Controller()     
        with pynput.mouse.Listener(on_click=on_click) as listener:
            listener.join()
            main()