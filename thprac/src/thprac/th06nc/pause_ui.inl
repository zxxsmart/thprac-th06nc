// Adapted from the existing TH06 pause menu.
    class THPauseMenu : public Gui::GameGuiWnd {
    public:
        THPauseMenu() noexcept
        {
            SetTitle("Pause Menu");
            SetFade(0.8f, 0.1f);
            SetSize(384.f, 448.f);
            SetPos(32.f, 16.f);
            SetItemWidth(-60);
            SetStyle(ImGuiStyleVar_WindowRounding, 0.0f);
            SetStyle(ImGuiStyleVar_WindowBorderSize, 0.0f);
        }

    public:

        bool el_bgm_signal { false };
        bool el_bgm_changed { false };

        enum state {
            STATE_CLOSE = 0,
            STATE_RESUME = 1,
            STATE_EXIT = 2,
            STATE_RESTART = 3,
            STATE_OPEN = 4,
        };
        enum signal {
            SIGNAL_NONE = 0,
            SIGNAL_RESUME = 1,
            SIGNAL_EXIT = 2,
            SIGNAL_RESTART = 3,
        };
        signal PMState()
        {
            switch (mState) {
            case THPauseMenu::STATE_CLOSE:
                return StateClose();
            case THPauseMenu::STATE_RESUME:
                return StateResume();
            case THPauseMenu::STATE_EXIT:
                return StateExit();
            case THPauseMenu::STATE_RESTART:
                return StateRestart();
            case THPauseMenu::STATE_OPEN:
                return StateOpen();
            default:
                return SIGNAL_NONE;
            }
        }

    protected:
        signal StateRestart()
        {
            if (mState != STATE_RESTART) {
                mState = STATE_RESTART;
                mFrameCounter = 0;
            }

            if (mFrameCounter == 1) {
                *mNavFocus = 0;
                inSettings = false;

                PracticeUi().State(5);

                Close();
            } else if (mFrameCounter == 10) {
                StateClose();
                return SIGNAL_RESTART;
            }

            return SIGNAL_NONE;
        }
        signal StateExit()
        {
            if (mState != STATE_EXIT) {
                mState = STATE_EXIT;
                mFrameCounter = 0;
            }

            if (mFrameCounter == 1) {
                *mNavFocus = 0;
                inSettings = false;
                Close();
            } else if (mFrameCounter == 10) {
                StateClose();
                return SIGNAL_EXIT;
            }

            return SIGNAL_NONE;
        }
        signal StateResume()
        {
            if (mState != STATE_RESUME) {
                mState = STATE_RESUME;
                mFrameCounter = 0;
            }

            if (mFrameCounter == 1) {
                *mNavFocus = 0;
                inSettings = false;
                Close();
            } else if (mFrameCounter == 10) {
                StateClose();
                return SIGNAL_RESUME;
            }

            return SIGNAL_NONE;
        }
        signal StateClose()
        {
            if (mState != STATE_CLOSE) {
                mState = STATE_CLOSE;
                mFrameCounter = 0;
            }

            if (mFrameCounter > 5) {
                return StateOpen();
            }

            return SIGNAL_NONE;
        }
        signal StateOpen()
        {
            if (mState != STATE_OPEN) {
                mState = STATE_OPEN;
                mFrameCounter = 0;
            }

            if (mFrameCounter == 1) {
                Open();
            }
            if (mFrameCounter > 10) {
                if (Gui::KeyboardInputGetSingle(VK_ESCAPE))
                    StateResume();
                else if (Gui::KeyboardInputGetRaw(0x51))
                    StateExit();
                else if (Gui::KeyboardInputGetRaw(0x52))
                    StateRestart();
            }

            return SIGNAL_NONE;
        }

        virtual void OnPreUpdate() override
        {
            if (mFrameCounter < UINT_MAX)
                mFrameCounter++;
        }
        virtual void OnLocaleChange() override
        {
            switch (Gui::LocaleGet()) {
            case LOCALE_ZH_CN:
                SetItemWidth(-60.0f);
                break;
            case LOCALE_EN_US:
                SetItemWidth(-60.0f);
                break;
            case LOCALE_JA_JP:
                SetItemWidth(-65.0f);
                break;
            default:
                break;
            }
        }
        virtual void OnContentUpdate() override
        {
            if (!inSettings) {
                ImGui::Dummy(ImVec2(10.0f, 140.0f));
                ImGui::Indent(119.0f);
                if (mResume())
                    StateResume();
                ImGui::Spacing();
                if (mExit())
                    StateExit();
                ImGui::Spacing();
                if (mRestart())
                    StateRestart();
                ImGui::Spacing();
                if (mSettings())
                    inSettings = !inSettings;
                ImGui::Spacing();
                ImGui::Unindent();
                mNavFocus();
            } else {
                ImGui::Dummy(ImVec2(10.0f, 10.0f));
                ImGui::Indent(119.0f);
                if (mResume())
                    StateResume();
                ImGui::Spacing();
                if (mExit())
                    StateExit();
                ImGui::Spacing();
                if (mRestart())
                    StateRestart();
                ImGui::Spacing();
                if (mSettings())
                    inSettings = !inSettings;
                ImGui::Spacing();
                ImGui::Unindent(67.0f);
                PracticeUi().PracticeMenu(mNavFocus);
            }
        }

        // Var
        state mState = STATE_CLOSE;
        unsigned int mFrameCounter = 0;
        bool inSettings = false;

        Gui::GuiButton mResume { TH_RESUME, 130.0f, 25.0f };
        Gui::GuiButton mExit { TH_EXIT, 130.0f, 25.0f };
        Gui::GuiButton mRestart { TH_RESTART, 130.0f, 25.0f };
        Gui::GuiButton mSettings { TH_TWEAK, 130.0f, 25.0f };

        Gui::GuiNavFocus mNavFocus { TH_RESUME, TH_EXIT, TH_RESTART, TH_TWEAK,
            TH_STAGE, TH_MODE, TH_WARP,
            TH_MID_STAGE, TH_END_STAGE, TH_NONSPELL, TH_SPELL, TH_PHASE,
            TH_LIFE, TH_BOMB, TH_SCORE, TH_POWER, TH_GRAZE, TH_POINT,
            TH06_RANK, TH06_RANKLOCK, TH06_FS };
    };
