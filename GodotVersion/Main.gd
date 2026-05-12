extends Control

@onready var mic_recorder = $MicRecorder
@onready var runtime_player = $RuntimeAudioPlayer
@onready var record_button = $UI/VBoxContainer/RecordButton
@onready var play_button = $UI/VBoxContainer/PlayButton
@onready var load_button = $UI/VBoxContainer/LoadFolderButton

func _ready():
	record_button.pressed.connect(_on_record_pressed)
	play_button.pressed.connect(_on_play_pressed)
	load_button.pressed.connect(_on_load_pressed)

func _on_record_pressed():
	mic_recorder.start_mic_capture()

func _on_play_pressed():
	if runtime_player.loaded_sounds.size() > 0:
		# Play the first loaded sound
		runtime_player.stream = runtime_player.loaded_sounds[0]
		runtime_player.play()
		runtime_player.set_current_playing_index(0)
	else:
		print("No WAV files loaded. Click 'Load WAVs from Folder' first or add .wav files to res://audio/")

func _on_load_pressed():
	# Example: load from audio folder
	runtime_player.load_wavs_from_folder("res://audio/")
