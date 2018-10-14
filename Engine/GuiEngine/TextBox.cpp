#include "TextBox.hpp"

#include "Layout.hpp"
#include "Placeholders/PlaceholderOverlayEntity.hpp"
#include "Placeholders/PlaceholderTextEntity.hpp"
#include <BaseLibrary/Platform/System.hpp>


namespace inl::gui {


TextBox::TextBox() {
	m_text.reset(new PlaceholderTextEntity());
	m_text->SetText({});
	m_frame.reset(new PlaceholderOverlayEntity());
	m_background.reset(new PlaceholderOverlayEntity());
	m_cursor.reset(new PlaceholderOverlayEntity());
	m_text->SetZDepth(0.1f);
	m_background->SetZDepth(0.0f);

	SetScripts();
}

void TextBox::SetSize(Vec2u size) {
	m_frame->SetScale(size);
	m_background->SetScale(Vec2(size) - Vec2(2, 2));
	m_text->SetSize(size);
}

Vec2u TextBox::GetSize() const {
	return m_frame->GetScale();
}

void TextBox::SetPosition(Vec2i position) {
	m_frame->SetPosition(position);
	m_background->SetPosition(position);
	m_text->SetPosition(position);
}
Vec2i TextBox::GetPosition() const {
	return m_frame->GetPosition();
}


void TextBox::Update(float elapsed) {
	SetColor();

	m_sinceLastCursorBlink += elapsed;
	if (m_sinceLastCursorBlink > 2*m_blinkTime) {
		m_sinceLastCursorBlink = 0.0f;
	}
	float alpha = m_sinceLastCursorBlink < m_blinkTime ? 1.0f : 0.0f;
	auto currentColor = m_cursor->GetColor();
	m_cursor->SetColor((Vec3)currentColor | alpha);

	auto font = GetStyle().font;
	auto fontSize = m_text->GetFontSize();
	if (font) {
		std::u32string u32text = EncodeString<char32_t>(GetText()) + U"_";
		auto [left, right] = font->FindCoordinates(u32text, m_cursorPosition, fontSize);
		m_cursor->SetScale({ 2, fontSize });
		m_cursor->SetPosition({ m_text->GetPosition().x + left - m_text->CalculateTextWidth()/2, m_text->GetPosition().y });
	}
}

void TextBox::SetText(std::string text) {
	m_cursorPosition = std::min(m_cursorPosition, (intptr_t)text.size());
	m_text->SetText(std::move(text));
}
const std::string& TextBox::GetText() const {
	return m_text->GetText();
}

void TextBox::SetZOrder(int rank) {
	m_frame->SetZDepth(rank);
	m_background->SetZDepth(rank + 0.1f);
	m_text->SetZDepth(rank + 0.2f);
	m_cursor->SetZDepth(rank + 0.3f);
}

std::vector<std::reference_wrapper<std::unique_ptr<gxeng::ITextEntity>>> TextBox::GetTextEntities() {
	return { m_text };
}

std::vector<std::reference_wrapper<std::unique_ptr<gxeng::IOverlayEntity>>> TextBox::GetOverlayEntities() {
	return { m_frame, m_background, m_cursor };
}


void TextBox::SetColor() {
	ColorF foreground;
	switch (GetState()) {
	case eStandardControlState::DEFAULT: foreground = GetStyle().foreground; break;
	case eStandardControlState::MOUSEOVER: foreground = GetStyle().hover; break;
	case eStandardControlState::FOCUSED: foreground = GetStyle().focus; break;
	case eStandardControlState::PRESSED: foreground = GetStyle().pressed; break;
	}
	m_frame->SetColor(foreground.v);
	m_background->SetColor(GetStyle().background.v);
	m_text->SetColor(GetStyle().text.v);
	m_cursor->SetColor(GetStyle().accent.v);
}


void TextBox::SetScripts() {
	OnCharacter += [this](char32_t character) {
		// Filter unwanted characters: backspace, delete, CR, LF, tab, vtab 
		if (std::u32string(U"\u0008\u007F\r\n\t\v").find(character) != std::u32string::npos) {
			return;
		}
		std::string text = GetText();
		if (m_cursorPosition >= text.size()) {
			m_cursorPosition = text.size();
			text.push_back((char)character);
		}
		else {
			text.insert(text.begin() + m_cursorPosition, (char)character);
		}
		++m_cursorPosition;
		SetText(text);
		m_sinceLastCursorBlink = 0.0f;
	};
	OnKeydown += [this](eKey key) {	
		std::string text = GetText();
		if (key == eKey::LEFT) {
			--m_cursorPosition;
			m_cursorPosition = std::max((intptr_t)0, m_cursorPosition);
		}
		else if (key == eKey::RIGHT) {
			++m_cursorPosition;
			m_cursorPosition = std::min(m_cursorPosition, (intptr_t)text.size());
		}
		else if (key == eKey::BACKSPACE && m_cursorPosition > 0) {
			assert(m_cursorPosition <= text.size());
			text.erase(text.begin() + m_cursorPosition - 1);
			--m_cursorPosition;
			SetText(text);
		}
		else if (key == eKey::DELETE && m_cursorPosition < (intptr_t)text.size()) {
			assert(m_cursorPosition <= text.size());
			text.erase(text.begin() + m_cursorPosition);
			SetText(text);
		}
		else if (key == eKey::HOME) {
			m_cursorPosition = 0;
		}
		else if (key == eKey::END) {
			m_cursorPosition = text.size();
		}
		m_sinceLastCursorBlink = 0.0f;
	};
	OnEnterArea += [] {
		System::SetCursorVisual(eCursorVisual::IBEAM, nullptr);
	};
	OnLeaveArea += [] {
		System::SetCursorVisual(eCursorVisual::ARROW, nullptr);
	};
}


} // namespace inl::gui