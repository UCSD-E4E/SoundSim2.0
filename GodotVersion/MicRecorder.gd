extends AudioStreamPlayer

@export var record_seconds: float = 5.0  # Equivalent to RecordSeconds
@export var sample_rate_hint: int = 48000  # For CSV timestamps

var capture_effect: AudioEffectCapture
var is_recording: bool = false
var timer: Timer

func _ready():
	# Get the capture bus (assuming bus 1 is "MicCapture")
	var bus_index = AudioServer.get_bus_index("MicCapture")
	if bus_index == -1:
		push_error("MicCapture bus not found. Create a bus named 'MicCapture' with AudioEffectCapture.")
		return
	capture_effect = AudioServer.get_bus_effect(bus_index, 0) as AudioEffectCapture

	# Set up a timer for stopping recording
	timer = Timer.new()
	add_child(timer)
	timer.one_shot = true
	timer.timeout.connect(_on_timer_timeout)

func start_mic_capture():
	if not capture_effect:
		push_error("MicCapture bus not set up. Add AudioEffectCapture to a bus.")
		return

	capture_effect.clear_buffer()  # Clear previous data
	is_recording = true
	timer.start(record_seconds)
	print("Mic capture started for ", record_seconds, " seconds.")

func _on_timer_timeout():
	stop_mic_capture()

func stop_mic_capture():
	if not is_recording:
		return

	is_recording = false
	var captured_frames = capture_effect.get_frames_available()
	var captured_data = capture_effect.get_buffer(captured_frames)

	if captured_data.size() == 0:
		print("No captured data.")
		return

	# captured_data is PackedVector2Array (stereo channels)
	# Convert to CSV (similar to your Unreal code)
	var csv = "sample_index,time_s,amplitude_norm\n"
	for i in range(captured_data.size()):
		# For stereo, we get Vector2(left, right). Average them or use one channel.
		var amplitude = clamp(captured_data[i].x, -1.0, 1.0)  # Use left channel
		var time_s = float(i) / sample_rate_hint if sample_rate_hint > 0 else 0.0
		csv += str(i) + "," + str(time_s) + "," + str(amplitude) + "\n"

	# Save to file (in user:// for portability)
	var dir = DirAccess.open("user://")
	if not dir:
		DirAccess.make_dir_absolute("user://MicCaptures")
	else:
		dir.make_dir("MicCaptures")
	var file_path = "user://MicCaptures/MicCapture_" + Time.get_datetime_string_from_system() + ".csv"
	var file = FileAccess.open(file_path, FileAccess.WRITE)
	if file:
		file.store_string(csv)
		file.close()
		print("Saved CSV to: ", file_path)
	else:
		push_error("Failed to save CSV.")

	# Optional: Play back the captured audio as PackedByteArray
	var audio_data = PackedByteArray()
	for sample in captured_data:
		# Convert float (-1.0 to 1.0) to 16-bit PCM
		var pcm_sample = int(clamp(sample.x, -1.0, 1.0) * 32767.0)
		audio_data.append_array(PackedByteArray([
			pcm_sample & 0xFF,
			(pcm_sample >> 8) & 0xFF
		]))

	var audio_stream = AudioStreamWAV.new()
	audio_stream.data = audio_data
	audio_stream.format = AudioStreamWAV.FORMAT_16_BITS
	audio_stream.mix_rate = sample_rate_hint
	audio_stream.stereo = false
	self.stream = audio_stream
	self.play()
