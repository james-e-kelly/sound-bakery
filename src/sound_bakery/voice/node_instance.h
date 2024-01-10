#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
	class Bus;
	class Container;
	class Node;
	class Sound;
	class SoundContainer;
	class Voice;

	/**
	 * @brief NodeInstances represent runtime versions of Nodes, either containers or busses
	*/
	class NodeInstance : public SB::Core::Object
	{
	public:
		NodeInstance();
		~NodeInstance();

	public:
		void setSoundInstance(SoundContainer* soundContainer, Sound* sound) noexcept;
		void setNodeInstance(Container* container) noexcept;
		void setBusInstance(Bus* bus) noexcept;

		void createParent();
		void createParentBusInstance();
		void createParentInstance();
		void createMasterBusParent();

		void init();
		void createChannelGroup();
		void bindDelegates();
		void createDSP();
		void attachToParent();

	public:
		bool isPlaying() const;

	public:
		void setVolume(float oldVolume, float newVolume);
		void setPitch(float oldPitch, float newPitch);
		void setLowpass(float oldLowpass, float newLowpass);
		void setHighpass(float oldHighpass, float newHighpass);

	public:
		SB::Core::DatabasePtr<Node> getReferencingNode() const noexcept { return m_referencingNode; }
		std::weak_ptr<NodeInstance> getParent() const noexcept { return m_parent; }
		SC_NODE_GROUP* getBus() const noexcept { return m_nodeGroup.get(); }

	private:
		SB::Core::DatabasePtr<Node> m_referencingNode;
		SB::Core::DatabasePtr<SoundContainer> m_referencingSoundNode;
		SB::Core::DatabasePtr<Bus> m_referencingBus;

		SC_DSP* m_lowpass = nullptr;
		SC_DSP* m_highpass = nullptr;

	private:
		std::unique_ptr<SC_NODE_GROUP, SC_NODE_GROUP_DELETER> m_nodeGroup;
		std::shared_ptr<NodeInstance> m_parent;

		RTTR_ENABLE(SB::Core::Object)
	};
}