# SoundSim Godot

Ported from Unreal Engine to Godot 4.6.2.

## Features
- Microphone recording with CSV export.
- Runtime WAV playback and batch loading.
- CSV logging of audio playback.

## Setup
1. Open the project in Godot 4.6.2.
2. In Audio > Buses, create a bus named "MicCapture" and add AudioEffectCapture.
3. Run the main scene.

## Usage
- Use the MicRecorder node to capture mic input.
- Use RuntimeAudioPlayer for playing WAV files.