# ipc_bridge.gd -- Autoload script in Godot project
#
# Add this as an autoload in your Godot project settings.
# It connects to the Qt host application over TCP on localhost
# and exchanges newline-delimited JSON messages.
extends Node

var peer := StreamPeerTCP.new()
var connected := false
var read_buffer := ""
var ipc_port := 9876

func _ready():
    # Parse the IPC port from command line
    for arg in OS.get_cmdline_user_args():
        if arg.begins_with("--ipc-port="):
            ipc_port = int(arg.split("=")[1])

    # Connect to Qt's TCP server on localhost
    peer.connect_to_host("127.0.0.1", ipc_port)

func _process(_delta):
    peer.poll()

    if peer.get_status() == StreamPeerTCP.STATUS_CONNECTED:
        if not connected:
            connected = true
            _on_connected()

        # Read incoming data
        var available := peer.get_available_bytes()
        if available > 0:
            read_buffer += peer.get_utf8_string(available)
            _process_buffer()

func _on_connected():
    # Send window handle to Qt for embedding
    var hwnd = DisplayServer.window_get_native_handle(
        DisplayServer.WINDOW_HANDLE
    )
    _send({"event": "ready", "window_handle": hwnd})

func _process_buffer():
    # Newline-delimited JSON parsing
    while true:
        var newline_pos := read_buffer.find("\n")
        if newline_pos < 0:
            break

        var line := read_buffer.substr(0, newline_pos)
        read_buffer = read_buffer.substr(newline_pos + 1)

        if line.is_empty():
            continue

        var msg = JSON.parse_string(line)
        if msg:
            _handle_command(msg)

func _handle_command(msg: Dictionary):
    var cmd: String = msg.get("cmd", "")
    var params: Dictionary = msg.get("params", {})

    match cmd:
        "set_satellite_position":
            var pos = params.get("pos", [0, 0, 0])
            var node = get_tree().root.find_child(params["id"], true, false)
            if node and node is Node3D:
                node.position = Vector3(pos[0], pos[1], pos[2])

        "resize_viewport":
            DisplayServer.window_set_size(
                Vector2i(params["width"], params["height"])
            )

        "shutdown":
            get_tree().quit()

func _send(data: Dictionary):
    var json := JSON.stringify(data) + "\n"
    peer.put_data(json.to_utf8_buffer())
