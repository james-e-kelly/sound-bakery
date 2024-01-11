#include <rttr/registration>

#include "sound_bakery/system.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/container/blend_container.h"
#include "sound_bakery/node/container/switch_container.h"
#include "sound_bakery/node/container/random_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/sound/sound.h"

RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace SB::Core;
    using namespace SB::Engine;

    registration::enumeration<SC_DSP_TYPE>("SC_DSP_TYPE")
        (
            value("Fader", SC_DSP_TYPE_FADER),
            value("Lowpass", SC_DSP_TYPE_LOWPASS),
            value("Highpass", SC_DSP_TYPE_HIGHPASS),
            value("Delay", SC_DSP_TYPE_DELAY)
        );

    registration::enumeration<SB_OBJECT_CATEGORY>("SB_OBJECT_CATEGORY")
        (
            value("Sound", SB_CATEGORY_SOUND),
            value("Bus", SB_CATEGORY_BUS),
            value("Node", SB_CATEGORY_NODE),
            value("Music", SB_CATEGORY_MUSIC),
            value("Event", SB_CATEGORY_EVENT),
            value("Parameter", SB_CATEGORY_PARAMETER)
        );

    registration::enumeration<SB_ACTION_TYPE>("SB_ACTION_TYPE")
        (
            value("Play", SB_ACTION_PLAY),
            value("Stop", SB_ACTION_STOP)
        );

    registration::class_<Action>("SB::Engine::Action")
        .constructor<>()
        (
            policy::ctor::as_object
        )
        .property("Type", &Action::m_type)
        .property("Destination", &Action::m_destination)
        (
            metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadObject)
        );

    registration::class_<EffectDescription>("SB::Engine::EffectDescription")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Type", &EffectDescription::getDSPType, &EffectDescription::setDSPType)
        .property("Parameters", &EffectDescription::m_parameterDescriptions);

    registration::class_<EffectParameterDescription>("SB::Engine::EffectParameterDescription")
        .constructor<>()
        (
            policy::ctor::as_object
        )
        .property("Parameter", &EffectParameterDescription::m_parameter)
        ;

    registration::class_<System>("SB::Engine::System")
        .property("MasterBus", &System::m_masterBus)
        ;

    registration::class_<Object>("SB::Core::Object")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        ;

    registration::class_<DatabaseObject>("SB::Core::DatabaseObject")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        // Order is important here!
        // ID must be set first
        // When loading the object name, the ID must be valid for the name->ID lookup
        .property("ObjectID", &DatabaseObject::getDatabaseID, &DatabaseObject::setDatabaseID)
        (
            metadata(SB::Editor::METADATA_KEY::Readonly, true)
        )
        .property("ObjectName", &DatabaseObject::getDatabaseName, &DatabaseObject::setDatabaseName)
        ;

    registration::class_<Sound>("SB::Engine::Sound")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Sound", &Sound::getSoundName, &Sound::setSoundName)
        (
            metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadSound)
        );

    registration::class_<NodeBase>("SB::Engine::NodeBase")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("ParentNode", &NodeBase::m_parentNode)
        (
            metadata(SB::Editor::METADATA_KEY::Readonly, true)
        )
        .property("OutputBus", &NodeBase::m_outputBus)
        (
            metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadBus)
        )
        .property("ChildNodes", &NodeBase::m_childNodes)
        (
            metadata(SB::Editor::METADATA_KEY::Readonly, true)
        )
        ;

    registration::class_<Node>("SB::Engine::Node")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Volume", &Node::m_volume)
        (
            metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 1.0f))
        )
        .property("Pitch", &Node::m_pitch)
        (
            metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 2.0f))
        )
        .property("Lowpass", &Node::m_lowpass)
        (
            metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f))
        )
        .property("Highass", &Node::m_highpass)
        (
            metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f))
        )
        .property("Effects", &Node::m_effectDescriptions)
        .method("Add Effect", &Node::addEffect)
        (
            parameter_names("Type")
        )
        ;

    registration::class_<Container>("SB::Engine::Container");

    registration::class_<SoundContainer>("SB::Engine::SoundContainer")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Sound", &SoundContainer::m_sound)
        (
            metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadSound)
        )
        ;
    
    registration::class_<BlendContainer>("SB::Engine::BlendContainer")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        ;

    registration::class_<SwitchContainer>("SB::Engine::SwitchContainer")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Switch", &SwitchContainer::getSwitchParameter, &SwitchContainer::setSwitchParameter)
        (
            metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadIntParam)
        )
        .property("Mappings", &SwitchContainer::getSwitchToChildMap, &SwitchContainer::setSwitchToChild)
        ;
    
    registration::class_<RandomContainer>("SB::Engine::RandomContainer")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        ;

    registration::class_<Bus>("SB::Engine::Bus")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property_readonly("IsMasterBus", &Bus::m_masterBus)
        ;

    registration::class_<Event>("SB::Engine::Event")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Actions", &Event::m_actions)
        ;

    registration::class_<FloatParameter>("SB::Engine::FloatParameter")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        ;

    registration::class_<IntParameter>("SB::Engine::IntParameter")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("ParameterValue", &IntParameter::getSelectedValue, &IntParameter::setSelectedValue)
        .property("Values", &IntParameter::m_values)
        (
            metadata(SB::Editor::METADATA_KEY::Readonly, true)
        )
        ;

    registration::class_<IntParameterValue>("SB::Engine::IntParameterValue")
        .constructor<>()
        (
            policy::ctor::as_raw_ptr
        )
        .property("Parent", &IntParameterValue::m_parentParameter)
        (
            metadata(SB::Editor::METADATA_KEY::Readonly, true)
        )
        ;
}