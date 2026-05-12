extends AudioStreamPlayer

@export var audio_file_path: String  # Single file path
@export var csv_output_folder: String = "user://SoundSimCSV"
@export var recording_interval: float = 1.0

var loaded_sounds: Array = []  # Array of AudioStreamWAV
var loaded_file_paths: Array = []
var current_playing_index: int = -1
var csv_timer: Timer
var csv_file: FileAccess
var is_recording: bool = false

func _ready():
	csv_timer = Timer.new()
	add_child(csv_timer)
	csv_timer.timeout.connect(_write_csv_row)

# Load and play a single WAV
func play_wav_from_file(file_path: String) -> bool:
	var audio_stream = load_wav_from_file(file_path)
	if not audio_stream:
		return false
	self.stream = audio_stream
	self.play()
	return true

# Load a WAV without playing
func load_wav_from_file(file_path: String) -> AudioStreamWAV:
	if not FileAccess.file_exists(file_path):
		print("File not found: ", file_path)
		return null

	var file = FileAccess.open(file_path, FileAccess.READ)
	var data = file.get_buffer(file.get_length())
	file.close()

	var parsed = parse_wav_data(data)
	if parsed.pcm.size() == 0:
		return null

	var audio_stream = AudioStreamWAV.new()
	audio_stream.data = parsed.pcm
	audio_stream.format = AudioStreamWAV.FORMAT_16_BITS
	audio_stream.mix_rate = parsed.sample_rate
	audio_stream.stereo = parsed.num_channels > 1
	return audio_stream

# Batch load from folder
func load_wavs_from_folder(folder_path: String, recursive: bool = true) -> Array:
	loaded_sounds.clear()
	loaded_file_paths.clear()

	var dir = DirAccess.open(folder_path)
	if not dir:
		print("Folder not found: ", folder_path)
		return []

	var files = []
	if recursive:
		files = _get_files_recursive(dir, folder_path)
	else:
		files = dir.get_files()
		for f in files:
			if f.ends_with(".wav"):
				files.append(folder_path + "/" + f)

	for f in files:
		var stream = load_wav_from_file(f)
		if stream:
			loaded_sounds.append(stream)
			loaded_file_paths.append(f)

	return loaded_sounds

# Custom WAV parser (simplified; handles 16/24/32-bit like your code)
func parse_wav_data(data: PackedByteArray) -> Dictionary:
	# Basic RIFF/WAVE check (similar to your ParseWavFile)
	if data.size() < 44 or data.slice(0, 4) != "RIFF".to_utf8_buffer() or data.slice(8, 12) != "WAVE".to_utf8_buffer():
		return {"pcm": PackedByteArray(), "sample_rate": 0, "num_channels": 0}

	# Find 'fmt ' chunk
	var fmt_offset = -1
	for i in range(12, data.size() - 8):
		if data.slice(i, i+4) == "fmt ".to_utf8_buffer():
			fmt_offset = i
			break

	if fmt_offset == -1:
		return {"pcm": PackedByteArray(), "sample_rate": 0, "num_channels": 0}

	var _num_channels = data.decode_u16(fmt_offset + 10)
	var _sample_rate = data.decode_u32(fmt_offset + 12)
	var bits_per_sample = data.decode_u16(fmt_offset + 22)

	# Find 'data' chunk
	var data_offset = -1
	for i in range(12, data.size() - 8):
		if data.slice(i, i+4) == "data".to_utf8_buffer():
			data_offset = i + 8
			break

	if data_offset == -1:
		return {"pcm": PackedByteArray(), "sample_rate": 0, "num_channels": 0}

	var pcm_data = data.slice(data_offset, data.size())

	# Convert to 16-bit (like your ConvertTo16Bit)
	var final_pcm = PackedByteArray()
	if bits_per_sample == 16:
		final_pcm = pcm_data
	elif bits_per_sample == 24:
		for i in range(0, pcm_data.size(), 3):
			final_pcm.append(pcm_data[i+1])
			final_pcm.append(pcm_data[i+2])
	elif bits_per_sample == 32:
		for i in range(0, pcm_data.size(), 4):
			final_pcm.append(pcm_data[i+2])
			final_pcm.append(pcm_data[i+3])
	else:
		return {"pcm": PackedByteArray(), "sample_rate": 0, "num_channels": 0}

	return {"pcm": final_pcm, "sample_rate": _sample_rate, "num_channels": _num_channels}

# CSV recording
func start_csv_recording():
	if is_recording:
		return

	var dir = DirAccess.open(csv_output_folder)
	if not dir:
		DirAccess.make_dir_absolute(csv_output_folder)

	var file_path = csv_output_folder + "/soundsim_" + Time.get_datetime_string_from_system() + ".csv"
	csv_file = FileAccess.open(file_path, FileAccess.WRITE)
	if csv_file:
		csv_file.store_string("Timestamp,GameTime,CurrentAudioFile\n")
		is_recording = true
		csv_timer.start(recording_interval)
	else:
		push_error("Failed to open CSV file.")

func stop_csv_recording():
	if not is_recording:
		return
	csv_timer.stop()
	if csv_file:
		csv_file.close()
	is_recording = false

func set_current_playing_index(index: int):
	current_playing_index = index

func _write_csv_row():
	if not is_recording or not csv_file:
		return

	var timestamp = Time.get_datetime_string_from_system()
	var game_time = Time.get_ticks_msec() / 1000.0
	var audio_file = "None"
	if current_playing_index >= 0 and current_playing_index < loaded_file_paths.size():
		audio_file = loaded_file_paths[current_playing_index].get_file()

	csv_file.store_string(timestamp + "," + str(game_time) + "," + audio_file + "\n")

# Helper for recursive file search
func _get_files_recursive(dir: DirAccess, path: String) -> Array:
	var files = []
	dir.list_dir_begin()
	var file_name = dir.get_next()
	while file_name != "":
		if dir.current_is_dir():
			var sub_dir = DirAccess.open(path + "/" + file_name)
			if sub_dir:
				files += _get_files_recursive(sub_dir, path + "/" + file_name)
		elif file_name.ends_with(".wav"):
			files.append(path + "/" + file_name)
		file_name = dir.get_next()
	return files
