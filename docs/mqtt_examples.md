
MQTT Examples
Topics

Input: iot/model/input

Output: iot/model/predictions

Example input (publish)
{"temp":25,"light":200,"magnet":1,"alarm":0}

Example output (subscribe)

Format depends on the running model server. Typical output includes predicted actions like heater/cooler/window/flood.
