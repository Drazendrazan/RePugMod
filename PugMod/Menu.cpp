#include "precompiled.h"

CMenu gMenu[33];

void CMenu::Clear()
{
	this->m_Text.clear();

	this->m_Data.clear();

	this->m_Info.clear();

	this->m_Skip.clear();

	this->m_Page = -1;

	this->m_Exit = false;

	this->m_Func = nullptr;
}

void CMenu::Create(std::string Title, bool Exit, void* CallbackFunction)
{
	this->Clear();

	this->m_Text = Title;

	this->m_Page = -1;

	this->m_Exit = Exit;

	this->m_Func = CallbackFunction; 
}

void CMenu::AddItem(int Item, std::string Text, bool Disabled)
{
	this->m_Data.push_back(Text);

	this->m_Info.push_back(Item);

	this->m_Skip.push_back(Disabled);
}

void CMenu::AddList(std::vector<std::string> List)
{
	this->m_Data.clear();

	this->m_Info.clear();

	this->m_Skip.clear();

	for (size_t i = 0; i < List.size(); i++)
	{
		this->AddItem(i, List[i].c_str(), false);
	}
}

void CMenu::Show(int EntityIndex)
{
	if (this->m_Data.size() && this->m_Info.size())
	{
		this->Display(EntityIndex, 0);
	}
}

void CMenu::Hide(int EntityIndex)
{
	this->m_Page = -1;

	this->HideMenu(EntityIndex);
}

bool CMenu::Handle(int EntityIndex, int Key)
{
	if (this->m_Page != -1)
	{
		auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

		if (Player)
		{
			if (Player->m_iMenu == Menu_OFF)
			{
				if (Key == 9)
				{
					this->Display(EntityIndex, ++this->m_Page);
				}
				else if (Key == 10)
				{
					this->Display(EntityIndex, --this->m_Page);
				}
				else
				{
					unsigned int ItemIndex = (Key + (this->m_Page * MENU_PAGE_OPTION)) - 1;

					if (ItemIndex < this->m_Info.size())
					{
						this->Hide(EntityIndex);

						if (this->m_Func)
						{
							((void(*)(int, int, bool, const char*))this->m_Func)(EntityIndex, this->m_Info[ItemIndex], this->m_Skip[ItemIndex], this->m_Data[ItemIndex].c_str());
						}
					}
				}

				return true;
			}
			else
			{
				this->m_Page = -1;
			}
		}
	}

	return false;
}

void CMenu::Display(int EntityIndex, int Page)
{
	if (Page < 0)
	{
		Page = 0;

		this->m_Page = 0;

		if (this->m_Exit)
		{
			this->m_Page = -1;
			return;
		}
	}
	else
	{
		this->m_Page = Page;
	}

	unsigned int Start = (Page * MENU_PAGE_OPTION);

	if (Start >= this->m_Data.size())
	{
		Start = Page = this->m_Page = 0;
	}

	int PageCount = this->m_Data.size() > MENU_PAGE_OPTION ? (this->m_Data.size() / MENU_PAGE_OPTION + ((this->m_Data.size() % MENU_PAGE_OPTION) ? 1 : 0)) : 1;

	std::string MenuText = "\\y" + this->m_Text;

	if (PageCount > 1)
	{
		MenuText += "\\R";
		MenuText += std::to_string(Page + 1);
		MenuText += "/";
		MenuText += std::to_string(PageCount);
	}

	MenuText += "\n\\w\n";
	
	unsigned int End = (Start + MENU_PAGE_OPTION);

	if (End > this->m_Data.size())
	{
		End = this->m_Data.size();
	}

	int Slots = (1 << 9); // MENU_KEY_0
	int BitSum = 0;

	for (unsigned int b = Start; b < End; b++)
	{
		Slots |= (1 << BitSum);

		MenuText += "\\r";
		MenuText += std::to_string(++BitSum);
		MenuText += this->m_Skip[b] ? ".\\d " : ".\\w ";
		MenuText += this->m_Data[b];
		MenuText += "\n";
	}

	if (End != this->m_Data.size())
	{
		Slots |= (1 << 8); // MENU_KEY_9; 

		if (Page)
		{
			MenuText += "\n\\r9.\\w Next\n\\r0.\\w Back";
		}
		else
		{
			MenuText += "\n\\r9.\\w Next";

			if (this->m_Exit)
			{
				MenuText += "\n\\r0.\\w Exit";
			}
		}
	}
	else
	{
		if (Page)
		{
			MenuText += "\n\\r0.\\w Back";
		}
		else
		{
			if (this->m_Exit)
			{
				MenuText += "\n\\r0.\\w Exit";
			}
		}
	}

	this->ShowMenu(EntityIndex, Slots, -1, MenuText);
}

void CMenu::ShowMenu(int EntityIndex, int Slots, int Time, std::string MenuText)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		if (!Player->IsDormant())
		{
			static int iMsgShowMenu;

			if (iMsgShowMenu || (iMsgShowMenu = GET_USER_MSG_ID(PLID, "ShowMenu", NULL)))
			{
				Player->m_iMenu = Menu_OFF;
				
				char BufferMenu[MAX_BUFFER_MENU * 6] = { 0 };

				MenuText.copy(BufferMenu, MenuText.length() + 1);

				char* pMenuList = BufferMenu;
				char* aMenuList = BufferMenu;

				int iCharCount = 0;

				while (pMenuList && *pMenuList)
				{
					char szChunk[MAX_BUFFER_MENU + 1] = {0};

					strncpy(szChunk, pMenuList, MAX_BUFFER_MENU);

					szChunk[MAX_BUFFER_MENU] = 0;

					iCharCount += strlen(szChunk);

					pMenuList = aMenuList + iCharCount;

					MESSAGE_BEGIN(MSG_ONE, iMsgShowMenu, nullptr, Player->edict());
					WRITE_SHORT(Slots);
					WRITE_CHAR(Time);
					WRITE_BYTE(*pMenuList ? TRUE : FALSE);
					WRITE_STRING(szChunk);
					MESSAGE_END();
				}
			}
		}
	}
}

void CMenu::HideMenu(int EntityIndex)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		if (!Player->IsDormant())
		{
			static int iMsgShowMenu;

			if (iMsgShowMenu || (iMsgShowMenu = GET_USER_MSG_ID(PLID, "ShowMenu", NULL)))
			{
				Player->m_iMenu = Menu_OFF;

				MESSAGE_BEGIN(MSG_ONE, iMsgShowMenu, nullptr, Player->edict());
				WRITE_SHORT(0);
				WRITE_CHAR(0);
				WRITE_BYTE(0);
				WRITE_STRING("");
				MESSAGE_END();
			}
		}
	}
}
