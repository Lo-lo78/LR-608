// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

// Pure, portable geometry of the Alt+D grid.  Keep the navigation rules
// independent of JUCE so GitHub Actions can regression-test them cheaply.
namespace lr608 {
constexpr int slotUiClamp(int value, int lower, int upper) noexcept
{
    return value < lower ? lower : value > upper ? upper : value;
}
constexpr int slotUiStart(int column) noexcept
{
    return slotUiClamp(column,0,3)*3;
}
constexpr int slotUiLength(int column) noexcept
{
    return column == 3 ? 1 : 3;
}
constexpr int slotUiNavigate(int index, int rowDelta, int columnDelta) noexcept
{
    const int currentColumn=slotUiClamp(index,0,9)/3;
    const int currentRow=slotUiClamp(index,0,9)-slotUiStart(currentColumn);
    const int nextColumn=slotUiClamp(currentColumn+columnDelta,0,3);
    const int nextRow=slotUiClamp(currentRow+(columnDelta==0?rowDelta:0),
                                  0,slotUiLength(nextColumn)-1);
    return slotUiStart(nextColumn)+nextRow;
}
constexpr bool slotUiReverseValueOrder(int cell) noexcept
{
    // Slot and Output are displayed in descending menu-item order.
    return cell==0 || cell==9;
}
} // namespace lr608
