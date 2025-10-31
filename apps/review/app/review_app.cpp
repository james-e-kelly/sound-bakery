#include "review_app.h"

#include "app/review_database.h"
#include "app/review_client.h"
#include "app/review_server.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "subsystems/video_subsystem.h"
#include "managers/intro_manager.h"
#include "managers/workspace_manager.h"
#include "widgets/review_root_widget.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

class review_app_drop_target : public IDropTarget
{
public:
    review_app_drop_target() : m_refCount(1) {}

    // IUnknown
    HRESULT QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == IID_IUnknown || riid == IID_IDropTarget)
        {
            *ppvObject = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG AddRef() override { return InterlockedIncrement(&m_refCount); }

    ULONG Release() override
    {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0)
            delete this;
        return count;
    }

    HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        *pdwEffect = DROPEFFECT_COPY;
        review_app::get()->m_isDragDropping = true;

        FORMATETC formatEtc = {
            .cfFormat = CF_HDROP, .ptd = nullptr, .dwAspect = DVASPECT_CONTENT, .lindex = -1, .tymed = TYMED_HGLOBAL};

        STGMEDIUM stgMedium;
        if (SUCCEEDED(pDataObj->GetData(&formatEtc, &stgMedium)))
        {
            HDROP hDrop = (HDROP)GlobalLock(stgMedium.hGlobal);
            if (hDrop != nullptr)
            {
                m_payloadPaths.clear();

                UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
                for (UINT i = 0; i < fileCount; ++i)
                {
                    char filePath[MAX_PATH];
                    if (DragQueryFile(hDrop, i, filePath, MAX_PATH))
                    {
                        std::filesystem::path path(filePath);
                        m_payloadPaths.insert(path);
                    }
                }
                GlobalUnlock(stgMedium.hGlobal);
            }
            ReleaseStgMedium(&stgMedium);
        }
        return S_OK;
    }

    HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        *pdwEffect = DROPEFFECT_COPY;

        return S_OK;
    }

    HRESULT DragLeave() override
    {
        review_app::get()->m_isDragDropping = false;
        return S_OK;
    }

    HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        *pdwEffect = DROPEFFECT_COPY;

        review_app::get()->m_isDragDropReady = true;

        return S_OK;
    }

    std::unordered_set<std::filesystem::path> m_payloadPaths;

private:
    ULONG m_refCount;
};

auto http_result_okay(const httplib::Result& result) -> bool
{
    return result && result.value().status == httplib::StatusCode::OK_200;
}

auto get_file_cache_directory() -> std::filesystem::path
{
    return std::filesystem::path(sago::getCacheDir()) / "SoundCheck" / "cache" / "downloaded";
}

auto create_application() -> gluten::app* 
{
	return new review_app();
}

auto review_app::pre_init() -> void
{
    m_databaseThread = make_worker_thread_executor();

    set_application_display_title("Sound Check");

    if (std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>())
    {
        if (std::shared_ptr<review_root_widget> rootWidget = widgetSubsystem->add_widget_class<review_root_widget>())
        {
            widgetSubsystem->set_root_widget(rootWidget);
        }
    }

    add_manager_class<intro_manager>();
}

auto review_app::post_init() -> void
{
    if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem =
            get_subsystem_by_class<gluten::renderer_subsystem>())
    {
        rendererSubsystem->set_maximised();
    }
}

auto review_app::exit() -> void
{
    if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem =
            get_subsystem_by_class<gluten::renderer_subsystem>())
    {
        RevokeDragDrop(glfwGetWin32Window(rendererSubsystem->get_glfw_window()));

        CoUninitialize();
    }
}

auto review_app::tick_implementation() -> void
{
    // We need to ensure we are sending the drag drop data every frame
    // The Windows callback is wonky and doesn't fire every frame
    if (m_dropTarget && get_is_drag_dropping())
    {
        if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem =
                gluten::app::get()->get_subsystem_by_class<gluten::renderer_subsystem>())
        {
            ImGuiIO& io = ImGui::GetIO();

            POINT cursorPos;
            GetCursorPos(&cursorPos);
            ScreenToClient(glfwGetWin32Window(rendererSubsystem->get_glfw_window()), &cursorPos);
            io.AddMousePosEvent((float)cursorPos.x, (float)cursorPos.y);
        }
    }
    static int dragDropReadyFrames = 0;

    if (m_isDragDropReady)
    {
        // Fallback if nothing took our drag drop files
        if (dragDropReadyFrames++ > 1)
        {
            m_isDragDropping = false;
            m_isDragDropReady = false;
        }
    }
    else
    {
        dragDropReadyFrames = 0;
    }
}

auto review_app::set_is_drag_dropping(bool dragDropping) -> void
{
    m_isDragDropping = dragDropping;
}

auto review_app::get() -> review_app*
{
    return static_cast<review_app*>(gluten::app::get());
}

auto review_app::get_database_thread_executor() const -> std::shared_ptr<concurrencpp::worker_thread_executor>
{
    return m_databaseThread;
}

auto review_app::get_review_database() -> std::shared_ptr<review_database>
{
    if (review_app* const app = get())
    {
        return app->m_database;
    }
    return nullptr;
}

auto review_app::create_review_database(const std::filesystem::path& path) -> void
{
    if (review_app* const app = get())
    {
        app->m_database = std::make_shared<review_database>(path);
    }
}

auto review_app::create_new_review_database(const std::filesystem::path& path) -> void
{
    if (review_app* const app = get())
    {
        std::filesystem::create_directories(path.parent_path());
        app->m_database = std::make_shared<review_database>(path);
        app->m_database->create_workspace(path.filename().string());
        setup_server(path);
    }
}

auto review_app::close_review_database() -> void
{
    if (review_app* const app = get())
    {
        app->m_database.reset();
    }
}

auto review_app::get_is_drag_dropping() const -> bool
{
    return m_isDragDropping;
}

auto review_app::get_drag_drop_files() const -> std::unordered_set<std::filesystem::path>
{
    if (m_isDragDropReady && m_dropTarget)
    {
        m_isDragDropping = false;
        m_isDragDropReady = false;
        return m_dropTarget->m_payloadPaths;
    }
    return {};
}

auto review_app::setup_client(const std::string& serverAddress) -> void
{
    if (review_app* const app = get())
    {
        gluten::data_source<user_settings_data> userSettings;
        userSettings->m_serverIpAddress = serverAddress;

        app->remove_manager_by_class<intro_manager>();
        app->m_dropTarget = std::make_unique<review_app_drop_target>();
        app->m_audioSubsystem = app->add_subsystem_class<gluten::audio_subsystem>();
        app->m_videoSubsystem = app->add_subsystem_class<video_subsystem>();
        std::shared_ptr<workspace_manager> workspaceManager = app->add_manager_class<workspace_manager>();
        workspaceManager->open_client(app->add_manager_class<review_client>(serverAddress));

        if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem = app->get_subsystem_by_class<gluten::renderer_subsystem>())
        {
            HRESULT r = OleInitialize(nullptr);
            assert(r == S_OK);

            HRESULT result = RegisterDragDrop(glfwGetWin32Window(rendererSubsystem->get_glfw_window()), app->m_dropTarget.get());
            assert(result == S_OK);
        }
    }
}

auto review_app::setup_server(const std::filesystem::path& workspaceFile) -> void
{
    if (review_app* const app = get())
    {
        gluten::data_source<user_settings_data> userSettings;
        userSettings->m_workspaceFilePath = workspaceFile;

        app->remove_manager_by_class<intro_manager>();
        review_app::create_review_database(workspaceFile);
        app->add_manager_class<review_server>(workspaceFile.parent_path());
    }
}

auto review_app::reset_to_intro() -> void
{
    if (review_app* const app = get())
    {
        gluten::data_source<user_settings_data> userSettings;
        userSettings->m_workspaceFilePath.clear();
        userSettings->m_loggedInUser.m_sessionToken.clear();

        app->remove_manager_by_class<review_server>();
        app->remove_manager_by_class<review_client>();
        app->remove_manager_by_class<workspace_manager>();

        app->add_manager_class<intro_manager>();
    }
}