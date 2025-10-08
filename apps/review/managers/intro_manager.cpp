#include "intro_manager.h"

#include "app/review_app.h"
#include "widgets/intro_widget.h"

namespace review_app_test_connect
{
	auto test_server_connection(std::string serverAddress) -> concurrencpp::result<bool>
	{
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        httplib::SSLClient testClient(serverAddress, 8080);
		testClient.enable_server_certificate_verification(false);

		httplib::Result testConnectionResult = testClient.Get(review_app_endpoints::me);

		co_return static_cast<bool>(testConnectionResult);
	}
}

auto intro_manager::init(gluten::app* app) -> void
{
	if (m_userSettingsData->server_address_valid())
	{
        m_testServerConnectionResult = review_app_test_connect::test_server_connection(m_userSettingsData->m_serverIpAddress);
		m_loadingPopup = app->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<gluten::loading_popup>(false);
        m_loadingPopup->open_popup();
		//review_app::setup_client(m_userSettingsData->m_serverIpAddress);
	}
    else if (m_userSettingsData->workspace_exists())
	{
        review_app::setup_server(m_userSettingsData->m_workspaceFilePath);
	}
	else
	{
		m_introWidget = gluten::add_widget_class_to_root<intro_widget>(false);
	}
}

auto intro_manager::start() -> void
{
	if (m_introWidget)
	{
        gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
		refresh.assign_widget_to_node(rttr::type::get<intro_widget>(), refresh.dockspaceID);
	}
}

auto intro_manager::tick(double deltaTime) -> void
{ 
	if (m_testServerConnectionResult)
	{
		if (m_testServerConnectionResult.status() == concurrencpp::result_status::value)
		{
			if (m_loadingPopup)
			{
				const bool serverConnectionOkay = m_testServerConnectionResult.get();

				if (serverConnectionOkay)
				{
					review_app::setup_server(m_userSettingsData->m_serverIpAddress);
				}
			}
		}
	}
	else if (m_loadingPopup)
	{
        m_loadingPopup.reset();
	}
    else if (!m_introWidget)
	{
		m_introWidget = gluten::add_widget_class_to_root<intro_widget>(false);
	}
}