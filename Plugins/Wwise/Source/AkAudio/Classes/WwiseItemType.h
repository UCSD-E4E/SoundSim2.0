/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2026 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#include "Containers/UnrealString.h"
#include "Engine/EngineTypes.h"

/**
 * @enum EWwiseItemType
 * @brief Enumeration representing different types of Wwise items in a Wwise project.
 *
 * This enum defines all the possible item types that can be found in the Wwise project hierarchy,
 * including events, busses, switches, states, and various container types.
 */
UENUM()
enum class EWwiseItemType
{
	Event,
	AuxBus,
	AcousticTexture,
	AudioDeviceShareSet,
	State,
	Switch,
	GameParameter,
	Trigger,
	EffectShareSet,
	ActorMixer,
	Bus, 
	Project,
	StandaloneWorkUnit,
	NestedWorkUnit,
	PhysicalFolder,
	Folder,
	Sound,
	SwitchContainer,
	RandomSequenceContainer,
	BlendContainer,
	MotionBus,
	StateGroup,
	SwitchGroup,
	InitBank,

	First = Event,
	Last = InitBank,
	LastWwiseBrowserType = EffectShareSet,

	None = -1,
};

/**
 * @brief Pre-increment operator for EWwiseItemType enumeration.
 *
 * Advances the enum value to the next item type. Used for iterating through item types.
 *
 * @param Item Reference to the EWwiseItemType value to increment.
 * @return Reference to the incremented EWwiseItemType value.
 * @note Asserts if incrementing beyond EWwiseItemType::Last.
 */
inline EWwiseItemType& operator++(EWwiseItemType& Item)
{
	using Underlying = std::underlying_type_t<EWwiseItemType>;
	auto Result = static_cast<Underlying>(Item) + 1;
	check(Result <= static_cast<Underlying>(EWwiseItemType::Last));
	Item = static_cast<EWwiseItemType>(Result);
	return Item;
}

/**
 * @brief Post-increment operator for EWwiseItemType enumeration.
 *
 * Advances the enum value to the next item type, returning the original value.
 * Used for iterating through item types.
 *
 * @param Item Reference to the EWwiseItemType value to increment.
 * @return The original EWwiseItemType value before incrementing.
 * @note Asserts if incrementing beyond EWwiseItemType::Last.
 */
inline EWwiseItemType operator++(EWwiseItemType& Item, int)
{
	auto Orig = Item;
	using Underlying = std::underlying_type_t<EWwiseItemType>;
	auto Result = static_cast<Underlying>(Item) + 1;
	check(Result <= static_cast<Underlying>(EWwiseItemType::Last));
	Item = static_cast<EWwiseItemType>(Result);
	return Orig;
}

/**
 * @namespace WwiseItemType
 * @brief Contains display names, folder names, and utility functions for Wwise item types.
 *
 * This namespace provides constants and helper functions for working with Wwise item types,
 * including human-readable display names for the browser UI and folder names that correspond
 * to the Wwise project structure on disk.
 */
namespace WwiseItemType
{
	static const FString EventsBrowserName = TEXT("Events");
	static const FString BussesBrowserName = TEXT("Busses");
	static const FString AcousticTexturesBrowserName = TEXT("Virtual Acoustics");
	static const FString AudioDeviceShareSetBrowserName = TEXT("Audio Device ShareSets");
	static const FString StatesBrowserName = TEXT("States");
	static const FString SwitchesBrowserName = TEXT("Switches");
	static const FString GameParametersBrowserName = TEXT("GameParameters");
	static const FString TriggersBrowserName = TEXT("Triggers");
	static const FString ShareSetsBrowserName =	TEXT("Effect ShareSets");
	static const FString OrphanAssetsBrowserName = TEXT("Orphan Assets");

	/**
	 * @brief Display names shown in the Wwise Browser UI.
	 *
	 * This array contains human-readable names for each category of Wwise items
	 * as they appear in the browser interface. The order corresponds to the
	 * browser tab ordering.
	 *
	 * @see EWwiseItemType
	 */
	static const FString BrowserDisplayNames[] = {
		EventsBrowserName,
		BussesBrowserName,
		AcousticTexturesBrowserName,
		AudioDeviceShareSetBrowserName,
		StatesBrowserName,
		SwitchesBrowserName,
		GameParametersBrowserName,
		TriggersBrowserName,
		ShareSetsBrowserName,
		OrphanAssetsBrowserName
	};

	/**
	 * @brief Tag names used to identify Work Units for each Wwise item type.
	 *
	 * This array contains the tag names found in Work Unit files that correspond
	 * to each Wwise item type. These tags are used when parsing Wwise project
	 * to determine the type of content within each Work Unit.
	 *
	 * @see EWwiseItemType
	 */
	static const FString WorkUnitTagNames[] = {
		TEXT("Events"),
		TEXT("Busses"),
		TEXT("VirtualAcoustics"),
		TEXT("AudioDevices"),
		TEXT("States"),
		TEXT("Switches"),
		TEXT("GameParameters"),
		TEXT("Triggers"),
		TEXT("Effects"),
	};
	
	/**
	 * @brief Folder names containing Work Units for each Wwise item type.
	 *
	 * This array maps to the actual folder names in the Wwise project directory
	 * structure where Work Units of each type are stored. These names may vary
	 * between Wwise versions.
	 *
	 * @see EWwiseItemType
	 */
	static const FString FolderNames[] = {
		TEXT("Events"),
		TEXT("Master-Mixer Hierarchy"),
		TEXT("Virtual Acoustics"),
		TEXT("Audio Devices"),
		TEXT("States"),
		TEXT("Switches"),
		TEXT("Game Parameters"),
		TEXT("Triggers"),
		TEXT("Effects")
	};

	/**
	 * @brief List of physical folder names to ignore when parsing the Wwise project structure.
	 *
	 * This array contains folder names that should be skipped during Wwise project
	 * traversal. These folders contain data that is either not relevant for the
	 * Unreal integration or is handled through different mechanisms.
	 *
	 * @see FolderNames
	 */
	static const TArray<FString> PhysicalFoldersToIgnore = {
		TEXT("Actor-Mixer Hierarchy"),
		TEXT("Attenuations"),
		TEXT("Control Surface Sessions"),
		TEXT("Conversion Settings"),
		TEXT("Dynamic Dialogue"),
		TEXT("Interactive Music Hierarchy"),
		TEXT("Metadata"),
		TEXT("Mixing Sessions"),
		TEXT("Modulators"),
		TEXT("Presets"),
		TEXT("Queries"),
		TEXT("SoundBanks"),
		TEXT("Soundcaster Sessions"),
	};

	/**
	 * @brief Converts a string representation to its corresponding EWwiseItemType value.
	 *
	 * Parses the given item name string and returns the matching enumeration value.
	 * This is useful for converting Wwise project type names to the internal enum representation.
	 *
	 * @param ItemName The string name of the Wwise item type (e.g., "Event", "AuxBus", "Sound").
	 * @return The corresponding EWwiseItemType value, or EWwiseItemType::None if not found.
	 *
	 * @code
	 * EWwiseItemType Type = WwiseItemType::FromString(TEXT("Event"));
	 * // Type == EWwiseItemType::Event
	 *
	 * EWwiseItemType InvalidType = WwiseItemType::FromString(TEXT("Unknown"));
	 * // InvalidType == EWwiseItemType::None
	 * @endcode
	 */
	inline EWwiseItemType FromString(const FString& ItemName)
	{
		struct TypePair
		{
			FString Name;
			EWwiseItemType Value;
		};

		static const TypePair ValidTypes[] = {
			{TEXT("AcousticTexture"), EWwiseItemType::AcousticTexture},
			{TEXT("AudioDevice"), EWwiseItemType::AudioDeviceShareSet},
			{TEXT("ActorMixer"), EWwiseItemType::ActorMixer},
			{TEXT("AuxBus"), EWwiseItemType::AuxBus},
			{TEXT("BlendContainer"), EWwiseItemType::BlendContainer},
			{TEXT("Bus"), EWwiseItemType::Bus},
			{TEXT("Event"), EWwiseItemType::Event},
			{TEXT("Folder"), EWwiseItemType::Folder},
			{TEXT("GameParameter"), EWwiseItemType::GameParameter},
			{TEXT("MotionBus"), EWwiseItemType::MotionBus},
			{TEXT("PhysicalFolder"), EWwiseItemType::PhysicalFolder},
			{TEXT("Project"), EWwiseItemType::Project},
			{TEXT("RandomSequenceContainer"), EWwiseItemType::RandomSequenceContainer},
			{TEXT("Sound"), EWwiseItemType::Sound},
			{TEXT("State"), EWwiseItemType::State},
			{TEXT("StateGroup"), EWwiseItemType::StateGroup},
			{TEXT("Switch"), EWwiseItemType::Switch},
			{TEXT("SwitchContainer"), EWwiseItemType::SwitchContainer},
			{TEXT("SwitchGroup"), EWwiseItemType::SwitchGroup},
			{TEXT("Trigger"), EWwiseItemType::Trigger},
			{TEXT("WorkUnit"), EWwiseItemType::StandaloneWorkUnit},
			{TEXT("Effect"), EWwiseItemType::EffectShareSet},
		};

		for (const auto& type : ValidTypes)
		{
			if (type.Name == ItemName)
			{
				return type.Value;
			}
		}

		return EWwiseItemType::None;
	}

	/**
	 * @brief Converts a Wwise project folder name to its corresponding EWwiseItemType value.
	 *
	 * Parses the given folder name string and returns the matching enumeration value.
	 * This is useful for determining the item type based on the physical folder structure
	 * of a Wwise project.
	 *
	 * @param ItemName The folder name from the Wwise project (e.g., "Events", "Master-Mixer Hierarchy").
	 * @return The corresponding EWwiseItemType value, or EWwiseItemType::None if not found.
	 *
	 * @see FromString
	 * @see FolderNames
	 */
	inline EWwiseItemType FromFolderName(const FString& ItemName)
	{
		struct TypePair
		{
			FString Name;
			EWwiseItemType Value;
		};

		static const TypePair ValidTypes[] = {
			{TEXT("Virtual Acoustics"), EWwiseItemType::AcousticTexture},
			{TEXT("Audio Devices"), EWwiseItemType::AudioDeviceShareSet},
			{TEXT("Master-Mixer Hierarchy"), EWwiseItemType::AuxBus},
			{TEXT("Events"), EWwiseItemType::Event},
			{TEXT("Game Parameters"), EWwiseItemType::GameParameter},
			{TEXT("States"), EWwiseItemType::State},
			{TEXT("Switches"), EWwiseItemType::Switch},
			{TEXT("Triggers"), EWwiseItemType::Trigger},
			{TEXT("Effects"), EWwiseItemType::EffectShareSet},
		};

		for (const auto& type : ValidTypes)
		{
			if (type.Name == ItemName)
			{
				return type.Value;
			}
		}

		return EWwiseItemType::None;
	}
}
