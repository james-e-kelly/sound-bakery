
/**
 * @page UserManual User Manual
 *
 * - @subpage Design
 * - @subpage Roadmap
 * - @subpage Guides
 */

/**
 * @page Guides Guides
 *
 * - @subpage UserGuide "User Guide"
 * - @subpage ProgrammerGuide "Programmer's Guide"
 */

/**
 * @page UserGuide User Guide
 */

/**
 * @page ProgrammerGuide Programmer's Guide
 *
 * @subpage ChefProgrammerGuide "Sound Chef Programmer Guide"
 */

/**
 * @page ChefProgrammerGuide Sound Chef Programmer's Guide
 *
 * Welcome to Sound Chef's programmer's guide. Sound Chef is a low level audio playback library suited for video games.
 * It can be thought of as an extension and wrapper of miniaudio. Miniaudio will be referenced heavily in this guide and
 * it is recommended to be familiar with the tool. You can read its docs
 * [here](https://miniaud.io/docs/manual/index.html).
 *
 * ## Creating The System Object
 *
 * sc_system objects can be created with calls to @ref sc_system_create. The system must be released by the user with a
 * call to @ref sc_system_release.
 *
 * Once created, call @ref sc_system_init to intialize the system and connect it to an audio output.
 *
 * @code
 * sc_system* system = NULL;
 * sc_system_create(&system);
 * sc_system_init(system);
 * @endcode
 *
 * The system manages the node graph, sounds, resources, and outputting audio to the audio device.
 *
 * ## Playing A Sound
 *
 * With a @ref sc_system object, playing a sound is incredibly easy.
 *
 * @code
 * sc_system* system = NULL;
 * sc_system_create(&system);
 * sc_system_init(system);
 *
 * sc_sound* sound = NULL;
 * sc_system_create_sound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * sc_sound_instance* instance = NULL;
 * sc_system_play_sound(system, sound, &instance, NULL, SC_FALSE);
 * @endcode
 *
 * Sound Chef seperates a loaded sound and a playing sound with the @ref sc_sound and @ref sc_sound_instance objects.
 * Internally, they are the same object and are simple extensions of the `ma_sound` object.
 *
 * This is done to ensure each call to @ref sc_system_play_sound creates a brand new sound with its play position set to
 * 0. If multiple calls to @ref sc_system_play_sound were made to the @ref sc_sound object, only one sound would be
 * heard and not many.
 *
 * Memory bloat isn't an issue here as miniaudio doesn't copy the loaded sound buffer but the pointers to that resource.
 * Miniaudio also handles reference counting and will release the resource when all sounds have finished playing and the
 * original @ref sc_sound is released.
 *
 * ## DSP
 *
 * For programs where the outcome is not predetermined, there is a need to abstract away strongly defined types. Sound
 * Bakery is a clear example as a user can insert many different types of effects into the signal chain; this would be
 * cumbersome if unique code was requried each time and for each effect.
 *
 * By abstracting effects into @ref sc_dsp objects, a user can quickly call functions like @ref sc_dsp_config_init and
 * @ref sc_system_create_dsp_by_desc to create many different effects with ease. Sound Chef handles initializing the underlying
 * `ma_*_node` objects.
 *
 * @note
 * Sound Chef uses many of the miniaudio effects like `ma_lpf_node`, `ma_hpf_node` etc. Refer to miniaudio's
 * documentation on how these work.
 *
 * With a DSP created, the user can call @ref sc_dsp_set_parameter_float to change properties of the effect on the fly.
 *
 * @code
 * const sc_dsp_config lpfConfig = sc_dsp_config_init(SC_DSP_TYPE_LOWPASS);
 * sc_dsp* lowpass = NULL;
 * sc_system_create_dsp_by_desc(system, &lpfConfig, &lowpass);
 *
 * sc_dsp_set_parameter_float(m_lowpass, SC_DSP_LOWPASS_PARAM_CUTOFF, 500.0f);
 * @endcode
 *
 * Many DSP parameters are defined in @ref sound_chef_dsp.h
 *
 * ## Node Groups
 *
 * Sound Chef adds the concept of "Node Groups". These are analogous to busses and Channels/ChannelControls in FMOD. The
 * intention is to create an easy API for inserting, removing, and modifying DSPs.
 *
 * To create a new @ref sc_node_group, call @ref sc_system_create_node_group. The created node group connects to the
 * endpoint/audio device by default.
 *
 * Sounds can be connected to node groups during calls to @ref sc_system_play_sound.
 *
 * @code
 * sc_system* system = NULL;
 * sc_system_create(&system);
 * sc_system_init(system);
 *
 * sc_sound* sound = NULL;
 * sc_system_create_sound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * sc_node_group* nodeGroup = NULL;
 * sc_system_create_node_group(system, &nodeGroup);
 *
 * sc_sound_instance* instance = NULL;
 * sc_system_play_sound(system, sound, &instance, nodeGroup, SC_FALSE);
 * @endcode
 *
 * DSP units can be inserted with ease.
 *
 * @code
 * sc_system* system = NULL;
 * sc_system_create(&system);
 * sc_system_init(system);
 *
 * sc_sound* sound = NULL;
 * sc_system_create_sound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * sc_node_group* nodeGroup = NULL;
 * sc_system_create_node_group(system, &nodeGroup);
 *
 * const sc_dsp_config lpfConfig = sc_dsp_config_init(SC_DSP_TYPE_LOWPASS);
 * sc_dsp* lowpass = NULL;
 * sc_system_create_dsp_by_desc(system, &lpfConfig, &lowpass);
 * sc_node_group_add_dsp(nodeGroup, lowpass, SC_DSP_INDEX_HEAD);
 *
 * sc_sound_instance* instance = NULL;
 * sc_system_play_sound(system, sound, &instance, nodeGroup, SC_FALSE);
 * @endcode
 *
 * Node Groups can also be connected together to create a complex graph.
 *
 * @code
 * sc_node_group* get_parent = NULL;
 * sc_node_group* child = NULL;
 *
 * sc_node_group_set_parent(child, get_parent);
 * @endcode
 *
 * It is recommended to refer to the @ref SB::Engine::node_instance class for how Sound Bakery uses Sound Chef to
 * connect node groups together, insert lowpass and highpass effects, and insert user-defined effects.
 *
 * ## Complete Example: Interactive Footstep System
 *
 * Here's a practical example combining sounds, effects, and node groups for an interactive audio system:
 *
 * @code
 * // Initialize system
 * sc_system* system = NULL;
 * sc_system_create(&system);
 * sc_system_init(system);
 *
 * // Load sounds
 * sc_sound* footstep_concrete = NULL;
 * sc_sound* footstep_dirt = NULL;
 * sc_system_create_sound(system, "footstep_concrete.wav", SC_SOUND_MODE_DEFAULT, &footstep_concrete);
 * sc_system_create_sound(system, "footstep_dirt.wav", SC_SOUND_MODE_DEFAULT, &footstep_dirt);
 *
 * // Create footstep bus with lowpass for simulation
 * sc_node_group* footstep_bus = NULL;
 * sc_system_create_node_group(system, &footstep_bus);
 *
 * // Add lowpass to footstep bus (for underwater footsteps, for example)
 * const sc_dsp_config lpf_config = sc_dsp_config_init(SC_DSP_TYPE_LOWPASS);
 * sc_dsp* lpf = NULL;
 * sc_system_create_dsp_by_desc(system, &lpf_config, &lpf);
 * sc_node_group_add_dsp(footstep_bus, lpf, SC_DSP_INDEX_HEAD);
 *
 * // Add compression to control dynamic range
 * const sc_dsp_config compressor_config = sc_dsp_config_init(SC_DSP_TYPE_COMPRESSOR);
 * sc_dsp* compressor = NULL;
 * sc_system_create_dsp_by_desc(system, &compressor_config, &compressor);
 * sc_node_group_add_dsp(footstep_bus, compressor, SC_DSP_INDEX_HEAD);
 *
 * // Play a footstep on concrete
 * sc_sound_instance* instance1 = NULL;
 * sc_system_play_sound(system, footstep_concrete, &instance1, footstep_bus, SC_FALSE);
 *
 * // Adjust the lowpass cutoff for environmental effect
 * sc_dsp_set_parameter_float(lpf, SC_DSP_LOWPASS_PARAM_CUTOFF, 2000.0f);
 *
 * // Play another footstep on dirt
 * sc_sound_instance* instance2 = NULL;
 * sc_system_play_sound(system, footstep_dirt, &instance2, footstep_bus, SC_FALSE);
 * @endcode
 *
 * ## Error Handling
 *
 * Most Sound Chef functions return @ref sc_result. Always check for errors:
 *
 * @code
 * sc_system* system = NULL;
 * sc_result result = sc_system_create(&system);
 * if (result != SC_SUCCESS) {
 *     // Handle error
 *     return;
 * }
 *
 * result = sc_system_init(system);
 * if (result != SC_SUCCESS) {
 *     // Handle error
 *     sc_system_release(&system);
 *     return;
 * }
 * @endcode
 *
 * ## Cleanup
 *
 * Always release resources when done:
 *
 * @code
 * // Stop sounds (instances are managed by Sound Chef)
 * // Release DSP effects if created manually
 * // Release node groups if created manually
 * // Finally, release the system
 * sc_system_release(&system);
 * @endcode
 *
 * ## Memory Management
 *
 * - @ref sc_sound objects represent loaded audio data and are reference-counted
 * - Multiple @ref sc_sound_instance objects can reference the same @ref sc_sound
 * - Releasing a @ref sc_sound does not stop playing instances; they continue until finished
 * - Node graphs and DSP effects are managed by the system and released with it
 * - For thread-safe operations, see @ref SECURITY.md
 *
 * ## Available DSP Effects
 *
 * Sound Chef provides these DSP effect types (see @ref sound_chef_dsp.h for parameter details):
 *
 * | Effect | Purpose | Common Parameters |
 * |--------|---------|-------------------|
 * | Lowpass (LPF) | Remove high frequencies | Cutoff, Resonance |
 * | Highpass (HPF) | Remove low frequencies | Cutoff, Resonance |
 * | Bandpass (BPF) | Keep frequencies in range | Cutoff, Resonance |
 * | Notch | Remove specific frequency | Cutoff, Resonance |
 * | Peaking | Boost/cut frequency range | Cutoff, Gain, Resonance |
 * | Compressor | Control dynamic range | Threshold, Ratio, Attack, Release |
 * | Reverb | Add spacious ambience | WetMix, DryMix, Decay |
 * | Delay | Create repeating echoes | DelayTime, Feedback, WetMix |
 * | Panning | Stereo positioning | Pan (-1.0 to 1.0) |
 * | Gating | Silence below threshold | Threshold, Attack, Release |
 *
 * Refer to miniaudio documentation for detailed effect behavior:
 * https://miniaud.io/docs/manual/index.html#DSP
 *
 * ## Performance Tips
 *
 * - **Reuse Sounds**: Create sounds once, play instances many times
 * - **Reuse Node Groups**: Many sounds can route through the same bus
 * - **Minimize Effects**: Each effect has CPU cost; only use what's needed
 * - **Parameter Updates**: Changing DSP parameters is fast; do it in game loop if needed
 * - **Streaming**: Use @ref SC_SOUND_MODE_STREAMING for large files to save memory
 *
 * @see COMPONENTS.md - Overview of Sound Chef vs Sound Bakery
 * @see SECURITY.md - Thread safety and audio callback constraints
 */