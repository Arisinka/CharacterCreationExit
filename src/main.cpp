#include "pch.h"

#include <array>
#include <string_view>

namespace
{
	constexpr std::uint32_t kEscapeKey = 0x01;
	constexpr double kSystemTab = 2.0;

	// Menus on top of character creation that handle Escape themselves
	const std::array<std::string_view, 2> kBlockingMenus{
		RE::MessageBoxMenu::MENU_NAME,
		RE::Console::MENU_NAME
	};

	bool IsBlockingMenuOpen(RE::UI* a_ui)
	{
		for (const auto name : kBlockingMenus) {
			if (a_ui->IsMenuOpen(name)) {
				return true;
			}
		}
		return false;
	}

	// The character name prompt is not a separate menu (no menu event is sent for it).
	// Assumption: while a text field is active the game counts it in the control map,
	// and Escape then stops working for a menu opened on top of it.
	int GetTextEntryCount()
	{
		const auto controlMap = RE::ControlMap::GetSingleton();
		// textEntryCount lives in the runtime data, whose offset differs between SE and AE
		return controlMap ? static_cast<int>(controlMap->GetRuntimeData().textEntryCount) : 0;
	}

	class CharacterCreationEscape :
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static CharacterCreationEscape& GetSingleton()
		{
			static CharacterCreationEscape instance;
			return instance;
		}

		bool Register()
		{
			const auto ui = RE::UI::GetSingleton();
			const auto input = RE::BSInputDeviceManager::GetSingleton();
			if (!ui || !input) {
				logger::critical("Cannot register: UI or input manager is unavailable");
				return false;
			}
			ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
			input->AddEventSink<RE::InputEvent*>(this);
			logger::info("Registered Escape handling for character creation");
			return true;
		}

		RE::BSEventNotifyControl ProcessEvent(
			const RE::MenuOpenCloseEvent* a_event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (a_event->menuName == RE::RaceSexMenu::MENU_NAME) {
				// Never keep a stale pending state across character creation sessions
				pending_ = false;
			}

			if (a_event->menuName == RE::JournalMenu::MENU_NAME) {
				if (a_event->opening && pending_) {
					SelectSystemTab(0);
				}
				else if (!a_event->opening) {
					pending_ = false;
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(
			RE::InputEvent* const* a_events,
			RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (!a_events || pending_) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto ui = RE::UI::GetSingleton();
			if (!ui || !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME) || IsBlockingMenuOpen(ui)) {
				return RE::BSEventNotifyControl::kContinue;
			}

			for (auto event = *a_events; event; event = event->next) {
				const auto button = event->AsButtonEvent();
				if (!button || button->GetDevice() != RE::INPUT_DEVICE::kKeyboard ||
					!button->IsDown() || button->GetIDCode() != kEscapeKey) {
					continue;
				}

				bool expected = false;
				if (!pending_.compare_exchange_strong(expected, true)) {
					break;
				}
				if (const auto tasks = SKSE::GetTaskInterface()) {
					// AddUITask already defers execution to the next UI update
					tasks->AddUITask([this] { OpenSystemMenu(); });
				}
				else {
					pending_ = false;
					logger::error("Cannot open the system menu: SKSE task interface is unavailable");
				}
				break;
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		void OpenSystemMenu()
		{
			if (!pending_) {
				return;
			}
			const auto ui = RE::UI::GetSingleton();
			if (!ui ||
				!ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME) ||
				ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) ||
				ui->IsMenuOpen(RE::MainMenu::MENU_NAME) ||
				IsBlockingMenuOpen(ui) ||
				GetTextEntryCount() > 0) {
				// Conditions changed between the key press and this UI update
				pending_ = false;
				return;
			}

			const auto queue = RE::UIMessageQueue::GetSingleton();
			if (!queue) {
				pending_ = false;
				logger::error("Cannot open the system menu: UI message queue is unavailable");
				return;
			}
			// Skyrim implements the pause/system menu as JournalMenu and uses
			// tab 2 for System.
			queue->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
			logger::info("Opening the system menu over character creation");
		}

		void SelectSystemTab(std::uint32_t a_attempt)
		{
			if (!pending_) {
				return;
			}
			const auto ui = RE::UI::GetSingleton();
			const auto movie = ui ? ui->GetMovieView(RE::JournalMenu::MENU_NAME) : nullptr;
			if (!movie) {
				// The movie may not exist yet, retry on the next UI update
				if (a_attempt < 30 && SKSE::GetTaskInterface()) {
					SKSE::GetTaskInterface()->AddUITask([this, attempt = a_attempt + 1] { SelectSystemTab(attempt); });
				}
				else {
					pending_ = false;
					logger::warn("Journal Menu did not expose its movie");
				}
				return;
			}

			RE::GFxValue args[2]{ RE::GFxValue(kSystemTab), RE::GFxValue(false) };
			if (!movie->Invoke("_root.QuestJournalFader.Menu_mc.RestoreSavedSettings", nullptr, args, 2)) {
				logger::warn("Could not select the Journal Menu System tab");
				return;
			}
			logger::info("Selected the System tab");
		}

		std::atomic<bool> pending_{ false };
	};

	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message && a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			if (!CharacterCreationEscape::GetSingleton().Register()) {
				logger::critical("Character creation Escape handling could not be registered");
			}
		}
	}
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .logName = "CharacterCreationExit" });
	logger::info("CharacterCreationExit loading");
	const auto messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::critical("Could not register SKSE message listener");
		return false;
	}
	return true;
}