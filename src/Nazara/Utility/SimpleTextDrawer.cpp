// Copyright (C) 2015 Jérôme Leclercq
// This file is part of the "Nazara Engine - Utility module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <memory>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	SimpleTextDrawer::SimpleTextDrawer() :
	m_color(Color::White),
	m_style(TextStyle_Regular),
	m_colorUpdated(true),
	m_glyphUpdated(true),
	m_characterSize(24)
	{
		SetFont(Font::GetDefault());
	}

	SimpleTextDrawer::SimpleTextDrawer(const SimpleTextDrawer& drawer) :
	m_color(drawer.m_color),
	m_text(drawer.m_text),
	m_style(drawer.m_style),
	m_colorUpdated(false),
	m_glyphUpdated(false),
	m_characterSize(drawer.m_characterSize)
	{
		SetFont(drawer.m_font);
	}

	SimpleTextDrawer::SimpleTextDrawer(SimpleTextDrawer&& drawer)
	{
		operator=(std::move(drawer));
	}

	SimpleTextDrawer::~SimpleTextDrawer() = default;

	void SimpleTextDrawer::AppendText(const String& str)
	{
		m_text.Append(str);
		if (m_glyphUpdated)
			GenerateGlyphs(str);
	}

	void SimpleTextDrawer::Clear()
	{
		m_text.Clear(true);
		ClearGlyphs();
	}

	const Recti& SimpleTextDrawer::GetBounds() const
	{
		if (!m_glyphUpdated)
			UpdateGlyphs();

		return m_bounds;
	}

	unsigned int SimpleTextDrawer::GetCharacterSize() const
	{
		return m_characterSize;
	}

	const Color& SimpleTextDrawer::GetColor() const
	{
		return m_color;
	}

	Font* SimpleTextDrawer::GetFont() const
	{
		return m_font;
	}

	Font* SimpleTextDrawer::GetFont(std::size_t index) const
	{
		NazaraAssert(index == 0, "Font index out of range");

		return m_font;
	}

	std::size_t SimpleTextDrawer::GetFontCount() const
	{
		return 1;
	}

	const AbstractTextDrawer::Glyph& SimpleTextDrawer::GetGlyph(std::size_t index) const
	{
		if (!m_glyphUpdated)
			UpdateGlyphs();
		else if (!m_colorUpdated)
			UpdateGlyphColor();

		return m_glyphs[index];
	}

	std::size_t SimpleTextDrawer::GetGlyphCount() const
	{
		if (!m_glyphUpdated)
			UpdateGlyphs();

		return m_glyphs.size();
	}

	UInt32 SimpleTextDrawer::GetStyle() const
	{
		return m_style;
	}

	const String& SimpleTextDrawer::GetText() const
	{
		return m_text;
	}

	void SimpleTextDrawer::SetCharacterSize(unsigned int characterSize)
	{
		m_characterSize = characterSize;

		m_glyphUpdated = false;
	}

	void SimpleTextDrawer::SetColor(const Color& color)
	{
		m_color = color;

		m_colorUpdated = false;
	}

	void SimpleTextDrawer::SetFont(Font* font)
	{
		if (m_font != font)
		{
			m_font = font;

			if (m_font)
				ConnectFontSlots();
			else
				DisconnectFontSlots();

			m_glyphUpdated = false;
		}
	}

	void SimpleTextDrawer::SetStyle(UInt32 style)
	{
		m_style = style;

		m_glyphUpdated = false;
	}

	void SimpleTextDrawer::SetText(const String& str)
	{
		m_text = str;

		m_glyphUpdated = false;
	}

	SimpleTextDrawer& SimpleTextDrawer::operator=(const SimpleTextDrawer& drawer)
	{
		m_characterSize = drawer.m_characterSize;
		m_color = drawer.m_color;
		m_style = drawer.m_style;
		m_text = drawer.m_text;

		m_colorUpdated = false;
		m_glyphUpdated = false;
		SetFont(drawer.m_font);

		return *this;
	}

	SimpleTextDrawer& SimpleTextDrawer::operator=(SimpleTextDrawer&& drawer)
	{
		DisconnectFontSlots();

		m_bounds = std::move(drawer.m_bounds);
		m_colorUpdated = std::move(drawer.m_colorUpdated);
		m_characterSize = std::move(drawer.m_characterSize);
		m_color = std::move(drawer.m_color);
		m_glyphs = std::move(drawer.m_glyphs);
		m_glyphUpdated = std::move(drawer.m_glyphUpdated);
		m_font = std::move(drawer.m_font);
		m_style = std::move(drawer.m_style);
		m_text = std::move(drawer.m_text);

		// Update slot pointers (TODO: Improve the way of doing this)
		ConnectFontSlots();

		return *this;
	}

	SimpleTextDrawer SimpleTextDrawer::Draw(const String& str, unsigned int characterSize, UInt32 style, const Color& color)
	{
		SimpleTextDrawer drawer;
		drawer.SetCharacterSize(characterSize);
		drawer.SetColor(color);
		drawer.SetStyle(style);
		drawer.SetText(str);

		return drawer;
	}

	SimpleTextDrawer SimpleTextDrawer::Draw(Font* font, const String& str, unsigned int characterSize, UInt32 style, const Color& color)
	{
		SimpleTextDrawer drawer;
		drawer.SetCharacterSize(characterSize);
		drawer.SetColor(color);
		drawer.SetFont(font);
		drawer.SetStyle(style);
		drawer.SetText(str);

		return drawer;
	}

	void SimpleTextDrawer::ClearGlyphs() const
	{
		m_bounds.MakeZero();
		m_colorUpdated = true;
		m_drawPos.Set(0, m_characterSize); //< Our draw "cursor"
		m_glyphs.clear();
		m_glyphUpdated = true;
		m_previousCharacter = 0;
		m_workingBounds.MakeZero(); //< Compute bounds as float to speedup bounds computation (as casting between floats and integers is costly)
	}

	void SimpleTextDrawer::ConnectFontSlots()
	{
		m_atlasChangedSlot.Connect(m_font->OnFontAtlasChanged, this, &SimpleTextDrawer::OnFontInvalidated);
		m_atlasLayerChangedSlot.Connect(m_font->OnFontAtlasLayerChanged, this, &SimpleTextDrawer::OnFontAtlasLayerChanged);
		m_fontReleaseSlot.Connect(m_font->OnFontRelease, this, &SimpleTextDrawer::OnFontRelease);
		m_glyphCacheClearedSlot.Connect(m_font->OnFontGlyphCacheCleared, this, &SimpleTextDrawer::OnFontInvalidated);
	}

	void SimpleTextDrawer::DisconnectFontSlots()
	{
		m_atlasChangedSlot.Disconnect();
		m_atlasLayerChangedSlot.Disconnect();
		m_fontReleaseSlot.Disconnect();
		m_glyphCacheClearedSlot.Disconnect();
	}

	void SimpleTextDrawer::GenerateGlyphs(const String& text) const
	{
		if (text.IsEmpty())
			return;

		///TODO: Allow iteration on Unicode characters without allocating any buffer
		std::u32string characters = text.GetUtf32String();
		if (characters.empty())
		{
			NazaraError("Invalid character set");
			return;
		}

		const Font::SizeInfo& sizeInfo = m_font->GetSizeInfo(m_characterSize);

		m_glyphs.reserve(m_glyphs.size() + characters.size());
		for (char32_t character : characters)
		{
			if (m_previousCharacter != 0)
				m_drawPos.x += m_font->GetKerning(m_characterSize, m_previousCharacter, character);

			m_previousCharacter = character;

			bool whitespace = true;
			switch (character)
			{
				case ' ':
					m_drawPos.x += sizeInfo.spaceAdvance;
					break;

				case '\n':
					m_drawPos.x = 0;
					m_drawPos.y += sizeInfo.lineHeight;
					break;

				case '\t':
					m_drawPos.x += sizeInfo.spaceAdvance * 4;
					break;

				default:
					whitespace = false;
					break;
			}

			if (whitespace)
				continue; // White spaces are blanks and invisible, move the draw position and skip the rest

			const Font::Glyph& fontGlyph = m_font->GetGlyph(m_characterSize, m_style, character);
			if (!fontGlyph.valid)
				continue; // Glyph failed to load, just skip it (can't do much)

			Glyph glyph;
			glyph.atlas = m_font->GetAtlas()->GetLayer(fontGlyph.layerIndex);
			glyph.atlasRect = fontGlyph.atlasRect;
			glyph.color = m_color;
			glyph.flipped = fontGlyph.flipped;

			int advance = fontGlyph.advance;

			Rectf bounds(fontGlyph.aabb);
			bounds.x += m_drawPos.x;
			bounds.y += m_drawPos.y;

			if (fontGlyph.requireFauxBold)
			{
				// Let's simulate bold by enlarging the glyph (not a neat idea, but should work)
				Vector2f center = bounds.GetCenter();

				// Enlarge by 10%
				bounds.width *= 1.1f;
				bounds.height *= 1.1f;

				// Replace it at the correct height
				Vector2f offset(bounds.GetCenter() - center);
				bounds.y -= offset.y;

				// Adjust advance (+10%)
				advance += advance / 10;
			}

			// We "lean" the glyph to simulate italics style
			float italic = (fontGlyph.requireFauxItalic) ? 0.208f : 0.f;
			float italicTop = italic * bounds.y;
			float italicBottom = italic * bounds.GetMaximum().y;

			glyph.corners[0].Set(bounds.x - italicTop, bounds.y);
			glyph.corners[1].Set(bounds.x + bounds.width - italicTop, bounds.y);
			glyph.corners[2].Set(bounds.x - italicBottom, bounds.y + bounds.height);
			glyph.corners[3].Set(bounds.x + bounds.width - italicBottom, bounds.y + bounds.height);

			if (!m_workingBounds.IsValid())
				m_workingBounds.Set(glyph.corners[0]);

			for (unsigned int i = 0; i < 4; ++i)
				m_workingBounds.ExtendTo(glyph.corners[i]);

			m_drawPos.x += advance;
			m_glyphs.push_back(glyph);
		}

		m_bounds.Set(Rectf(std::floor(m_workingBounds.x), std::floor(m_workingBounds.y), std::ceil(m_workingBounds.width), std::ceil(m_workingBounds.height)));
	}

	void SimpleTextDrawer::OnFontAtlasLayerChanged(const Font* font, AbstractImage* oldLayer, AbstractImage* newLayer)
	{
		NazaraUnused(font);

		#ifdef NAZARA_DEBUG
		if (m_font != font)
		{
			NazaraInternalError("Not listening to " + String::Pointer(font));
			return;
		}
		#endif

		// Update atlas layer pointer
		// Note: This can happen while updating
		for (Glyph& glyph : m_glyphs)
		{
			if (glyph.atlas == oldLayer)
				glyph.atlas = newLayer;
		}
	}

	void SimpleTextDrawer::OnFontInvalidated(const Font* font)
	{
		NazaraUnused(font);

		#ifdef NAZARA_DEBUG
		if (m_font != font)
		{
			NazaraInternalError("Not listening to " + String::Pointer(font));
			return;
		}
		#endif

		m_glyphUpdated = false;
	}

	void SimpleTextDrawer::OnFontRelease(const Font* font)
	{
		NazaraUnused(font);
		NazaraUnused(font);

		#ifdef NAZARA_DEBUG
		if (m_font != font)
		{
			NazaraInternalError("Not listening to " + String::Pointer(font));
			return;
		}
		#endif

		SetFont(nullptr);
	}

	void SimpleTextDrawer::UpdateGlyphColor() const
	{
		for (Glyph& glyph : m_glyphs)
			glyph.color = m_color;

		m_colorUpdated = true;
	}

	void SimpleTextDrawer::UpdateGlyphs() const
	{
		NazaraAssert(m_font && m_font->IsValid(), "Invalid font");

		ClearGlyphs();
		GenerateGlyphs(m_text);
	}
}
