#include <sstream>
#include <vector>
#include <cmath>
#include "PreviewView.h"
#include "gui/dialogues/TextPrompt.h"
#include "simulation/SaveRenderer.h"
#include "gui/interface/Point.h"
#include "gui/interface/Window.h"
#include "gui/interface/Textbox.h"
#include "gui/Style.h"
#include "Format.h"
#include "gui/search/Thumbnail.h"
#include "gui/profile/ProfileActivity.h"
#include "client/Client.h"
#include "gui/interface/ScrollPanel.h"
#include "gui/interface/AvatarButton.h"
#include "gui/interface/Keys.h"

class PreviewView::LoginAction: public ui::ButtonAction
{
	PreviewView * v;
public:
	LoginAction(PreviewView * v_){ v = v_; }
	virtual void ActionCallback(ui::Button * sender)
	{
		v->c->ShowLogin();
	}
};

class PreviewView::SubmitCommentAction: public ui::ButtonAction
{
	PreviewView * v;
public:
	SubmitCommentAction(PreviewView * v_){ v = v_; }
	virtual void ActionCallback(ui::Button * sender)
	{
		v->submitComment();
	}
};

class PreviewView::AutoCommentSizeAction: public ui::TextboxAction
{
	PreviewView * v;
public:
	AutoCommentSizeAction(PreviewView * v): v(v) {}
	virtual void TextChangedCallback(ui::Textbox * sender) {
		v->commentBoxAutoHeight();
	}
};

class PreviewView::AvatarAction: public ui::AvatarButtonAction
{
	PreviewView * v;
public:
	AvatarAction(PreviewView * v_){ v = v_; }
	virtual void ActionCallback(ui::AvatarButton * sender)
	{
		if(sender->GetUsername().size() > 0)
		{
			new ProfileActivity(sender->GetUsername());
		}
	}
};

PreviewView::PreviewView():
	ui::Window(ui::Point(-1, -1), ui::Point((XRES/2)+210, (YRES/2)+150)),
	savePreview(NULL),
	doOpen(false),
	addCommentBox(NULL),
	submitCommentButton(NULL),
	commentBoxHeight(20),
	showAvatars(true)
{
	class FavAction: public ui::ButtonAction
	{
		PreviewView * v;
	public:
		FavAction(PreviewView * v_){ v = v_; }
		virtual void ActionCallback(ui::Button * sender)
		{
			v->c->FavouriteSave();
		}
	};

	showAvatars = Client::Ref().GetPrefBool("ShowAvatars", true);

	favButton = new ui::Button(ui::Point(50, Size.Y-19), ui::Point(51, 19), "Fav");
	favButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;	favButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	favButton->SetIcon(IconFavourite);
	favButton->SetActionCallback(new FavAction(this));
	favButton->Enabled = Client::Ref().GetAuthUser().ID?true:false;
	AddComponent(favButton);

	class ReportPromptCallback: public TextDialogueCallback {
	public:
		PreviewView * v;
		ReportPromptCallback(PreviewView * v_) { v = v_;	}
		virtual void TextCallback(TextPrompt::DialogueResult result, std::string resultText) {
			if (result == TextPrompt::ResultOkay)
				v->c->Report(resultText);
		}
		virtual ~ReportPromptCallback() { }
	};

	class ReportAction: public ui::ButtonAction
	{
		PreviewView * v;
	public:
		ReportAction(PreviewView * v_){ v = v_; }
		virtual void ActionCallback(ui::Button * sender)
		{
			new TextPrompt("Report Save", "Things to consider when reporting:\n\bw1)\bg When reporting stolen saves, please include the ID of the original save.\n\bw2)\bg Do not waste staff time with fake or bogus reports, doing so may result in a ban.", "", "[reason]", true, new ReportPromptCallback(v));
		}
	};
	reportButton = new ui::Button(ui::Point(100, Size.Y-19), ui::Point(51, 19), "Report");
	reportButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;	reportButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	reportButton->SetIcon(IconReport);
	reportButton->SetActionCallback(new ReportAction(this));
	reportButton->Enabled = Client::Ref().GetAuthUser().ID?true:false;
	AddComponent(reportButton);

	class OpenAction: public ui::ButtonAction
	{
		PreviewView * v;
	public:
		OpenAction(PreviewView * v_){ v = v_; }
		virtual void ActionCallback(ui::Button * sender)
		{
			v->c->DoOpen();
		}
	};
	openButton = new ui::Button(ui::Point(0, Size.Y-19), ui::Point(51, 19), "Open");
	openButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;	openButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	openButton->SetIcon(IconOpen);
	openButton->SetActionCallback(new OpenAction(this));
	AddComponent(openButton);

	class BrowserOpenAction: public ui::ButtonAction
	{
		PreviewView * v;
	public:
		BrowserOpenAction(PreviewView * v_){ v = v_; }
		virtual void ActionCallback(ui::Button * sender)
		{
			v->c->OpenInBrowser();
		}
	};

	browserOpenButton = new ui::Button(ui::Point((XRES/2)-107, Size.Y-19), ui::Point(108, 19), "Open in browser");
	browserOpenButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;	browserOpenButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	browserOpenButton->SetIcon(IconOpen);
	browserOpenButton->SetActionCallback(new BrowserOpenAction(this));
	AddComponent(browserOpenButton);

	if(showAvatars)
		saveNameLabel = new ui::Label(ui::Point(39, (YRES/2)+4), ui::Point(100, 16), "");
	else
		saveNameLabel = new ui::Label(ui::Point(5, (YRES/2)+4), ui::Point(100, 16), "");
	saveNameLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	saveNameLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(saveNameLabel);

	if(showAvatars)
		saveDescriptionLabel = new ui::Label(ui::Point(5, (YRES/2)+4+15+21), ui::Point((XRES/2)-10, Size.Y-((YRES/2)+4+15+17)-25), "");
	else
		saveDescriptionLabel = new ui::Label(ui::Point(5, (YRES/2)+4+15+19), ui::Point((XRES/2)-10, Size.Y-((YRES/2)+4+15+17)-23), "");
	saveDescriptionLabel->SetMultiline(true);
	saveDescriptionLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	saveDescriptionLabel->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	saveDescriptionLabel->SetTextColour(ui::Colour(180, 180, 180));
	AddComponent(saveDescriptionLabel);

	if(showAvatars)
		authorDateLabel = new ui::Label(ui::Point(39, (YRES/2)+4+15), ui::Point(180, 16), "");
	else
		authorDateLabel = new ui::Label(ui::Point(5, (YRES/2)+4+15), ui::Point(200, 16), "");
	authorDateLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	authorDateLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(authorDateLabel);

	if(showAvatars)
	{
		avatarButton = new ui::AvatarButton(ui::Point(4, (YRES/2)+4), ui::Point(34, 34), "");
		avatarButton->SetActionCallback(new AvatarAction(this));
		AddComponent(avatarButton);
	}

	viewsLabel = new ui::Label(ui::Point((XRES/2)-80, (YRES/2)+4+15), ui::Point(80, 16), "");
	viewsLabel->Appearance.HorizontalAlign = ui::Appearance::AlignRight;
	viewsLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(viewsLabel);
	
	pageInfo = new ui::Label(ui::Point((XRES/2) + 85, Size.Y+1), ui::Point(70, 16), "Page 1 of 1");
	pageInfo->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	AddComponent(pageInfo);

	commentsPanel = new ui::ScrollPanel(ui::Point((XRES/2)+1, 1), ui::Point((Size.X-(XRES/2))-2, Size.Y-commentBoxHeight));
	AddComponent(commentsPanel);
}

void PreviewView::AttachController(PreviewController * controller)
{
	c = controller;

	int textWidth = Graphics::textwidth("Click the box below to copy the save ID");
	saveIDLabel = new ui::Label(ui::Point((Size.X-textWidth-20)/2, Size.Y+5), ui::Point(textWidth+20, 16), "Click the box below to copy the save ID");
	saveIDLabel->SetTextColour(ui::Colour(150, 150, 150));
	saveIDLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	AddComponent(saveIDLabel);

	textWidth = Graphics::textwidth(format::NumberToString<int>(c->SaveID()).c_str());
	saveIDLabel2 = new ui::Label(ui::Point((Size.X-textWidth-20)/2-37, Size.Y+22), ui::Point(40, 16), "Save ID:");
	AddComponent(saveIDLabel2);
	
	saveIDButton = new ui::CopyTextButton(ui::Point((Size.X-textWidth-10)/2, Size.Y+20), ui::Point(textWidth+10, 18), format::NumberToString<int>(c->SaveID()), saveIDLabel);
	AddComponent(saveIDButton);
}

void PreviewView::commentBoxAutoHeight()
{
	if(!addCommentBox)
		return;
	int textWidth = Graphics::textwidth(addCommentBox->GetText().c_str());
	if(textWidth+15 > Size.X-(XRES/2)-48)
	{
		addCommentBox->Appearance.VerticalAlign = ui::Appearance::AlignTop;

		int oldSize = addCommentBox->Size.Y;
		addCommentBox->AutoHeight();
		int newSize = addCommentBox->Size.Y+5;
		addCommentBox->Size.Y = oldSize;

		commentBoxHeight = newSize+22;
		commentBoxPositionX = (XRES/2)+4;
		commentBoxPositionY = Size.Y-(newSize+21);
		commentBoxSizeX = Size.X-(XRES/2)-8;
		commentBoxSizeY = newSize;
	}
	else
	{
		commentBoxHeight = 20;
		addCommentBox->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;

		commentBoxPositionX = (XRES/2)+4;
		commentBoxPositionY = Size.Y-19;
		commentBoxSizeX = Size.X-(XRES/2)-48;
		commentBoxSizeY = 17;
	}
	commentsPanel->Size.Y = Size.Y-commentBoxHeight;
}

void PreviewView::DoDraw()
{
	Window::DoDraw();
	Graphics * g = ui::Engine::Ref().g;
	for(int i = 0; i < commentTextComponents.size(); i++)
	{
		int linePos = commentTextComponents[i]->Position.Y+commentsPanel->ViewportPosition.Y+commentTextComponents[i]->Size.Y+4;
		if(linePos > 0 && linePos < Size.Y-commentBoxHeight)
		g->draw_line(
				Position.X+1+XRES/2,
				Position.Y+linePos,
				Position.X+Size.X-2,
				Position.Y+linePos,
				255, 255, 255, 100);
	}
	if(c->GetDoOpen())
	{
		g->fillrect(Position.X+(Size.X/2)-101, Position.Y+(Size.Y/2)-26, 202, 52, 0, 0, 0, 210);
		g->drawrect(Position.X+(Size.X/2)-100, Position.Y+(Size.Y/2)-25, 200, 50, 255, 255, 255, 180);
		g->drawtext(Position.X+(Size.X/2)-(Graphics::textwidth("Loading save...")/2), Position.Y+(Size.Y/2)-5, "Loading save...", style::Colour::InformationTitle.Red, style::Colour::InformationTitle.Green, style::Colour::InformationTitle.Blue, 255);
	}
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 255, 255, 255, 255);

}

void PreviewView::OnDraw()
{
	Graphics * g = ui::Engine::Ref().g;

	//Window Background+Outline
	g->clearrect(Position.X-2, Position.Y-2, Size.X+4, Size.Y+4);

	//Save preview (top-left)
	if(savePreview && savePreview->Buffer)
	{
		g->draw_image(savePreview, (Position.X+1)+(((XRES/2)-savePreview->Width)/2), (Position.Y+1)+(((YRES/2)-savePreview->Height)/2), 255);
	}
	g->drawrect(Position.X, Position.Y, (XRES/2)+1, (YRES/2)+1, 255, 255, 255, 100);
	g->draw_line(Position.X+XRES/2, Position.Y+1, Position.X+XRES/2, Position.Y+Size.Y-2, 200, 200, 200, 255);

	if(votesUp || votesDown)
	{
		float ryf;
		int nyu, nyd;
		int lv = (votesUp>votesDown)?votesUp:votesDown;
		lv = (lv>10)?lv:10;

		if (50>lv)
		{
			ryf = 50.0f/((float)lv);
			nyu = votesUp*ryf;
			nyd = votesDown*ryf;
		}
		else
		{
			ryf = ((float)lv)/50.0f;
			nyu = votesUp/ryf;
			nyd = votesDown/ryf;
		}
		nyu = nyu>50?50:nyu;
		nyd = nyd>50?50:nyd;

		g->fillrect(Position.X+(XRES/2)-55, Position.Y+(YRES/2)+3, 53, 7, 0, 107, 10, 255);
		g->fillrect(Position.X+(XRES/2)-55, Position.Y+(YRES/2)+9, 53, 7, 107, 10, 0, 255);
		g->drawrect(Position.X+(XRES/2)-55, Position.Y+(YRES/2)+3, 53, 7, 128, 128, 128, 255);
		g->drawrect(Position.X+(XRES/2)-55, Position.Y+(YRES/2)+9, 53, 7, 128, 128, 128, 255);

		g->fillrect(Position.X+(XRES/2)-4-nyu, Position.Y+(YRES/2)+5, nyu, 3, 57, 187, 57, 255);
		g->fillrect(Position.X+(XRES/2)-4-nyd, Position.Y+(YRES/2)+11, nyd, 3, 187, 57, 57, 255);
	}
}

void PreviewView::OnTick(float dt)
{
	if(addCommentBox)
	{
		ui::Point positionDiff = ui::Point(commentBoxPositionX, commentBoxPositionY)-addCommentBox->Position;
		ui::Point sizeDiff = ui::Point(commentBoxSizeX, commentBoxSizeY)-addCommentBox->Size;

		if(positionDiff.X!=0)
		{
			int xdiff = positionDiff.X/5;
			if(xdiff == 0)
				xdiff = 1*isign(positionDiff.X);
			addCommentBox->Position.X += xdiff;
		}
		if(positionDiff.Y!=0)
		{
			int ydiff = positionDiff.Y/5;
			if(ydiff == 0)
				ydiff = 1*isign(positionDiff.Y);
			addCommentBox->Position.Y += ydiff;
		}

		if(sizeDiff.X!=0)
		{
			int xdiff = sizeDiff.X/5;
			if(xdiff == 0)
				xdiff = 1*isign(sizeDiff.X);
			addCommentBox->Size.X += xdiff;
			addCommentBox->Invalidate();
		}
		if(sizeDiff.Y!=0)
		{
			int ydiff = sizeDiff.Y/5;
			if(ydiff == 0)
				ydiff = 1*isign(sizeDiff.Y);
			addCommentBox->Size.Y += ydiff;
			addCommentBox->Invalidate();
		}
	}

	c->Update();
}

void PreviewView::OnTryExit(ExitMethod method)
{
	c->Exit();
}

void PreviewView::OnMouseWheel(int x, int y, int d)
{
	if(commentsPanel->GetScrollLimit() == 1 && d < 0)
		c->NextCommentPage();
	if(commentsPanel->GetScrollLimit() == -1 && d > 0)
		c->PrevCommentPage();

}

void PreviewView::OnKeyPress(int key, Uint16 character, bool shift, bool ctrl, bool alt)
{
	if ((key == KEY_ENTER || key == KEY_RETURN) && (!addCommentBox || !addCommentBox->IsFocused()))
		openButton->DoAction();
}

void PreviewView::NotifySaveChanged(PreviewModel * sender)
{
	SaveInfo * save = sender->GetSave();
	if(savePreview)
		delete savePreview;
	savePreview = NULL;
	if(save)
	{
		votesUp = save->votesUp;
		votesDown = save->votesDown;
		saveNameLabel->SetText(save->name);
		if(showAvatars) {
			avatarButton->SetUsername(save->userName);
			authorDateLabel->SetText("\bw" + save->userName + " \bgDate:\bw " + format::UnixtimeToDateMini(save->date));
		}
		else
		{
			authorDateLabel->SetText("\bgAuthor:\bw " + save->userName + " \bgDate:\bw " + format::UnixtimeToDateMini(save->date));
		}
		viewsLabel->SetText("\bgViews:\bw " + format::NumberToString<int>(save->Views));
		saveDescriptionLabel->SetText(save->Description);
		if(save->Favourite)
		{
			favButton->Enabled = true;
			favButton->SetText("Unfav");
		}
		else if(Client::Ref().GetAuthUser().ID)
		{
			favButton->Enabled = true;
			favButton->SetText("Fav");
		}
		else
		{
			favButton->SetText("Fav");
			favButton->Enabled = false;
		}

		if(save->GetGameSave())
		{
			savePreview = SaveRenderer::Ref().Render(save->GetGameSave(), false, true);

			if(savePreview && savePreview->Buffer && !(savePreview->Width == XRES/2 && savePreview->Width == YRES/2))
			{
				int newSizeX, newSizeY;
				pixel * oldData = savePreview->Buffer;
				float factorX = ((float)XRES/2)/((float)savePreview->Width);
				float factorY = ((float)YRES/2)/((float)savePreview->Height);
				float scaleFactor = factorY < factorX ? factorY : factorX;
				savePreview->Buffer = Graphics::resample_img(oldData, savePreview->Width, savePreview->Height, savePreview->Width*scaleFactor, savePreview->Height*scaleFactor);
				delete[] oldData;
				savePreview->Width *= scaleFactor;
				savePreview->Height *= scaleFactor;
			}
		}
	}
	else
	{
		votesUp = 0;
		votesDown = 0;
		saveNameLabel->SetText("");
		authorDateLabel->SetText("");
		saveDescriptionLabel->SetText("");
		favButton->Enabled = false;
	}
}

void PreviewView::submitComment()
{
	if(addCommentBox)
	{
		std::string comment = std::string(addCommentBox->GetText());
		submitCommentButton->Enabled = false;
		addCommentBox->SetText("");
		addCommentBox->SetPlaceholder("Submitting comment"); //This doesn't appear to ever show since no separate thread is created
		FocusComponent(NULL);

		if (!c->SubmitComment(comment))
			addCommentBox->SetText(comment);

		addCommentBox->SetPlaceholder("Add comment");
		submitCommentButton->Enabled = true;

		commentBoxAutoHeight();
	}
}

void PreviewView::NotifyCommentBoxEnabledChanged(PreviewModel * sender)
{
	if(addCommentBox)
	{
		RemoveComponent(addCommentBox);
		delete addCommentBox;
		addCommentBox = NULL;
	}
	if(submitCommentButton)
	{
		RemoveComponent(submitCommentButton);
		delete submitCommentButton;
		submitCommentButton = NULL;
	}
	if(sender->GetCommentBoxEnabled())
	{
		commentBoxPositionX = (XRES/2)+4;
		commentBoxPositionY = Size.Y-19;
		commentBoxSizeX = Size.X-(XRES/2)-48;
		commentBoxSizeY = 17;

		addCommentBox = new ui::Textbox(ui::Point((XRES/2)+4, Size.Y-19), ui::Point(Size.X-(XRES/2)-48, 17), "", "Add Comment");
		addCommentBox->SetActionCallback(new AutoCommentSizeAction(this));
		addCommentBox->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		addCommentBox->SetMultiline(true);
		AddComponent(addCommentBox);
		submitCommentButton = new ui::Button(ui::Point(Size.X-40, Size.Y-19), ui::Point(40, 19), "Submit");
		submitCommentButton->SetActionCallback(new SubmitCommentAction(this));
		//submitCommentButton->Enabled = false;
		AddComponent(submitCommentButton);
	}
	else
	{
		submitCommentButton = new ui::Button(ui::Point(XRES/2, Size.Y-19), ui::Point(Size.X-(XRES/2), 19), "Login to comment");
		submitCommentButton->SetActionCallback(new LoginAction(this));
		AddComponent(submitCommentButton);
	}
}

void PreviewView::NotifyCommentsPageChanged(PreviewModel * sender)
{
	std::stringstream pageInfoStream;
	pageInfoStream << "Page " << sender->GetCommentsPageNum() << " of " << sender->GetCommentsPageCount();
	pageInfo->SetText(pageInfoStream.str());
}

void PreviewView::NotifyCommentsChanged(PreviewModel * sender)
{
	std::vector<SaveComment*> * comments = sender->GetComments();

	for(int i = 0; i < commentComponents.size(); i++)
	{
		commentsPanel->RemoveChild(commentComponents[i]);
		delete commentComponents[i];
	}
	commentComponents.clear();
	commentTextComponents.clear();
	commentsPanel->InnerSize = ui::Point(0, 0);

	if(comments)
	{
		for(int i = 0; i < commentComponents.size(); i++)
		{
			commentsPanel->RemoveChild(commentComponents[i]);
			delete commentComponents[i];
		}
		commentComponents.clear();
		commentTextComponents.clear();

		int currentY = 0;//-yOffset;
		ui::Label * tempUsername;
		ui::Label * tempComment;
		ui::AvatarButton * tempAvatar;
		for(int i = 0; i < comments->size(); i++)
		{
			int usernameY = currentY+5, commentY;
			if(showAvatars)
			{
				tempAvatar = new ui::AvatarButton(ui::Point(2, currentY+7), ui::Point(26, 26), comments->at(i)->authorName);
				tempAvatar->SetActionCallback(new AvatarAction(this));
				commentComponents.push_back(tempAvatar);
				commentsPanel->AddChild(tempAvatar);
			}

			if(showAvatars)
				tempUsername = new ui::Label(ui::Point(31, currentY+3), ui::Point(Size.X-((XRES/2) + 13), 16), comments->at(i)->authorNameFormatted);
			else
				tempUsername = new ui::Label(ui::Point(5, currentY+3), ui::Point(Size.X-((XRES/2) + 13), 16), comments->at(i)->authorNameFormatted);
			tempUsername->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
			tempUsername->Appearance.VerticalAlign = ui::Appearance::AlignBottom;
			currentY += 16;

			commentComponents.push_back(tempUsername);
			commentsPanel->AddChild(tempUsername);

			commentY = currentY+5;
			if(showAvatars)
				tempComment = new ui::Label(ui::Point(31, currentY+5), ui::Point(Size.X-((XRES/2) + 13 + 26), -1), comments->at(i)->comment);
			else
				tempComment = new ui::Label(ui::Point(5, currentY+5), ui::Point(Size.X-((XRES/2) + 13), -1), comments->at(i)->comment);
			tempComment->SetMultiline(true);
			tempComment->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
			tempComment->Appearance.VerticalAlign = ui::Appearance::AlignTop;
			tempComment->SetTextColour(ui::Colour(180, 180, 180));
			currentY += tempComment->Size.Y+4;

			commentComponents.push_back(tempComment);
			commentsPanel->AddChild(tempComment);
			commentTextComponents.push_back(tempComment);
		}

		commentsPanel->InnerSize = ui::Point(commentsPanel->Size.X, currentY+4);
	}
}

/*void PreviewView::NotifyPreviewChanged(PreviewModel * sender)
{
	savePreview = sender->GetGameSave();
	if(savePreview && savePreview->Data && !(savePreview->Width == XRES/2 && savePreview->Height == YRES/2))
	{
		int newSizeX, newSizeY;
		float factorX = ((float)XRES/2)/((float)savePreview->Width);
		float factorY = ((float)YRES/2)/((float)savePreview->Height);
		float scaleFactor = factorY < factorX ? factorY : factorX;
		savePreview->Data = Graphics::resample_img(savePreview->Data, savePreview->Width, savePreview->Height, savePreview->Width*scaleFactor, savePreview->Height*scaleFactor);
		savePreview->Width *= scaleFactor;
		savePreview->Height *= scaleFactor;
	}
}*/

PreviewView::~PreviewView()
{
	if(addCommentBox)
	{
		RemoveComponent(addCommentBox);
		delete addCommentBox;
	}
	if(submitCommentButton)
	{
		RemoveComponent(submitCommentButton);
		delete submitCommentButton;
	}
	if(savePreview)
		delete savePreview;
}

