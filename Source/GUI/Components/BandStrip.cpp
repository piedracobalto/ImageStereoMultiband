#include "BandStrip.h"

BandStrip::BandStrip(juce::AudioProcessorValueTreeState& apvts, int bandIndex)
    : bandNumber(bandIndex + 1),
    accentColour(BandColours::getBandColour(bandIndex))
{
    titleLabel.setText("Band " + juce::String(bandNumber), juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a9ba8));
    gainLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(gainLabel);

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider.setColour(juce::Slider::thumbColourId, accentColour);
    gainSlider.setColour(juce::Slider::trackColourId, accentColour);
    gainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff505a64));
    addAndMakeVisible(gainSlider);

    gainValueLabel.setJustificationType(juce::Justification::centred);
    gainValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    gainValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    gainValueLabel.setEditable(true, true);
    gainValueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff20242a));
    gainValueLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff15171a));
    gainValueLabel.setColour(juce::Label::outlineColourId, accentColour);
    gainValueLabel.onTextChange = [this]
        {
            gainSlider.setValue(gainValueLabel.getText().getFloatValue());
        };
    gainSlider.onValueChange = [this]
        {
            gainValueLabel.setText(juce::String(gainSlider.getValue(), 1), juce::dontSendNotification);
        };
    addAndMakeVisible(gainValueLabel);

    // ── Width slider (0–100, default 50) ────────────────────────────────────
    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    widthSlider.setColour(juce::Slider::thumbColourId, accentColour);
    widthSlider.setColour(juce::Slider::trackColourId, accentColour);
    widthSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff505a64));
    widthSlider.getProperties().set("widthSlider", true);
    widthSlider.addMouseListener(this, true);

    widthValueLabel.setJustificationType(juce::Justification::centred);
    widthValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    widthValueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff15171a));
    widthValueLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff15171a));
    widthValueLabel.setColour(juce::Label::outlineColourId, accentColour.withAlpha(0.5f));
    widthValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    // Show "50" before the attachment fires — matches the parameter default
    widthValueLabel.setText("50", juce::dontSendNotification);
    widthValueLabel.setVisible(true);
    widthValueLabel.setEditable(true, true);
    widthValueLabel.onTextChange = [this]
        {
            auto v = juce::jlimit(0, 100, (int)widthValueLabel.getText().getFloatValue());
            widthSlider.setValue((float)v);
        };
    addAndMakeVisible(widthValueLabel);

    // Keep label in sync whenever the slider moves (from attachment or mouse)
    // Display as integer 0–100
    widthSlider.onValueChange = [this]
        {
            widthValueLabel.setText(
                juce::String((int)std::round(widthSlider.getValue())),
                juce::dontSendNotification);
        };

    addAndMakeVisible(widthSlider);

    widthLabel.setText("width", juce::dontSendNotification);
    widthLabel.setJustificationType(juce::Justification::centred);
    widthLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a9ba8));
    widthLabel.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(widthLabel);

    muteButton.setClickingTogglesState(true);
    soloButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.85f));
    soloButton.setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.85f));
    muteButton.onClick = [this] { repaint(); };
    soloButton.onClick = [this] { repaint(); };
    addAndMakeVisible(muteButton);
    addAndMakeVisible(soloButton);

    // Attachments — these set the slider to the saved parameter value,
    // which triggers onValueChange and updates the label automatically.
    const auto bandPrefix = "band" + juce::String(bandNumber);
    widthAttachment = std::make_unique<SliderAttachment>(apvts, bandPrefix + "Width", widthSlider);
    gainAttachment = std::make_unique<SliderAttachment>(apvts, bandPrefix + "Gain", gainSlider);
    muteAttachment = std::make_unique<ButtonAttachment>(apvts, bandPrefix + "Mute", muteButton);
    soloAttachment = std::make_unique<ButtonAttachment>(apvts, bandPrefix + "Solo", soloButton);

    // Sync gain label after attachment sets the real value
    gainValueLabel.setText(juce::String(gainSlider.getValue(), 1), juce::dontSendNotification);
}

void BandStrip::paint(juce::Graphics& g)
{
    const bool isMuted = muteButton.getToggleState();
    const bool isSoloed = soloButton.getToggleState();
    const bool dimmed = isMuted || (hasOtherSolo && !isSoloed);
    const float alpha = dimmed ? 0.05f : 1.0f;

    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a).withAlpha(alpha));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff343941).withAlpha(alpha));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto accent = bounds.reduced(7.0f);
    accent.setHeight(3.0f);
    g.setColour(accentColour.withAlpha(alpha));
    g.fillRoundedRectangle(accent, 1.5f);
}

void BandStrip::setHasOtherSolo(bool v)
{
    hasOtherSolo = v;
    repaint();
}

void BandStrip::mouseEnter(const juce::MouseEvent&) {}
void BandStrip::mouseExit(const juce::MouseEvent&) {}

void BandStrip::resized()
{
    auto area = getLocalBounds().reduced(8);

    // ── Top: accent gap + title + "Gain" label ──────────────────────────────
    area.removeFromTop(8);
    titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    gainLabel.setBounds(area.removeFromTop(14));
    area.removeFromTop(4);

    // ── Bottom: "width" label → slider → value label (above slider) ─────────
    widthLabel.setBounds(area.removeFromBottom(14));
    auto widthSliderRow = area.removeFromBottom(26);
    widthSlider.setBounds(widthSliderRow.reduced(4, 2));
    area.removeFromBottom(2);

    constexpr int wValW = 52, wValH = 20;
    auto widthValRow = area.removeFromBottom(wValH);
    widthValueLabel.setBounds(widthValRow.withSizeKeepingCentre(wValW, wValH));
    area.removeFromBottom(4);

    // ── Right column: gainValueLabel + M + S, centred vertically ────────────
    constexpr int rightColW = 48;
    constexpr int valueBoxH = 24;
    constexpr int btnH = 30;
    constexpr int btnGap = 6;
    constexpr int blockH = valueBoxH + btnGap + btnH + btnGap + btnH;

    auto rightCol = area.removeFromRight(rightColW);
    int  blockY = rightCol.getY() + (rightCol.getHeight() - blockH) / 2;

    gainValueLabel.setBounds(rightCol.getX() + 2, blockY, rightColW - 4, valueBoxH);
    blockY += valueBoxH + btnGap;

    muteButton.setBounds(rightCol.getX() + 2, blockY, rightColW - 4, btnH);
    blockY += btnH + btnGap;

    soloButton.setBounds(rightCol.getX() + 2, blockY, rightColW - 4, btnH);

    // ── Gain slider: fills remaining area ───────────────────────────────────
    gainSlider.setBounds(area.withSizeKeepingCentre(area.getWidth(), area.getHeight()));
}