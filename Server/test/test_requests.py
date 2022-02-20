import requests
import time
import json

def reset(server_address:str, server_port:int, amp_value:int):
    payload = {'value': amp_value}
    r = requests.post(f"http://{server_address}:{server_port}/charger/amp", data=json.dumps(payload))

def main():
    server_address = '127.0.0.1'
    server_port = 8800
    amp_value = 6
    if False:
        while True:
            payload = {'value': amp_value}
            r = requests.post(f"http://{server_address}:{server_port}/charger/amp", data=json.dumps(payload))
            amp_value += 1
            if(amp_value > 20):
                amp_value = 6
            time.sleep(0.5)
    else:
        reset(server_address, server_port, amp_value)


if __name__ == '__main__':
    main()
