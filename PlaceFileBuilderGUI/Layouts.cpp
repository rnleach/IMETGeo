#include "Layouts.hpp"

#include <iostream>

namespace Win32Helper
{
  AbstractLayout::AbstractLayout(Expand expOpt, LytOpt lytOpt, Collapse clpsOpt) :
    expOpt_{ expOpt }, lytOpt_(lytOpt), clpsOpt_{ clpsOpt }, hAlign_{HorizontalAlignment::Center}, 
    vAlign_{VerticalAlignment::Center}, hidden_ { false }, 
    heightCache_{ nullVal }, widthCache_{ nullVal }
  {}

  Coord AbstractLayout::requestHeight()
  {
    if (hidden_ && clpsOpt_ == Collapse::Yes) return 0;
    if (heightCache_ == nullVal) refreshCache();
    return heightCache_;
  }

  Coord AbstractLayout::requestWidth()
  {
    if (hidden_ && clpsOpt_ == Collapse::Yes) return 0;
    if (widthCache_ == nullVal) refreshCache();
    return widthCache_;
  }

  bool AbstractLayout::willExpandVertical()
  {
    return expOpt_ == Expand::Vertical || expOpt_ == Expand::Both;
  }

  bool AbstractLayout::willExpandHorizontal()
  {
    return expOpt_ == Expand::Horizontal || expOpt_ == Expand::Both;
  }

  void AbstractLayout::resetCache()
  {
    widthCache_ = heightCache_ = nullVal;
  }

  SingleControlLayout::SingleControlLayout(HWND control, LytOpt lytOpt,
    Expand exOpt, Collapse clpsOpt) :
    AbstractLayout(exOpt, lytOpt, clpsOpt), hwnd_{ control } {}

  SingleControlLayout::~SingleControlLayout() {}

  void SingleControlLayout::refreshCache()
  {
    // Is this a static control?
    wchar_t buffer[MAX_PATH];
    GetClassNameW(hwnd_, buffer, MAX_PATH);
    bool isStatic = wcsncmp(buffer, L"Static", MAX_PATH) == 0;
    
    // Get the window style information
    DWORD dwxStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd_, GWL_EXSTYLE));

    // Use window style information to choose message to send to get border sizes
    int borderV = 0, borderH = 0;
    if (!isStatic)
    {
      borderV = (dwxStyle && WS_EX_CLIENTEDGE) ? SM_CYEDGE : SM_CYBORDER;
      borderH = (dwxStyle && WS_EX_CLIENTEDGE) ? SM_CXEDGE : SM_CXBORDER;
      borderV = GetSystemMetrics(borderV);
      borderH = GetSystemMetrics(borderH);
    }

    // Get the text in the control
    // re-use buffer from above
    GetWindowTextW(hwnd_, buffer, MAX_PATH);
    if (wcsnlen_s(buffer, MAX_PATH) == 0) wcsncpy(buffer, L"Empty String Error.", MAX_PATH);

    // Get the font
    HFONT hFont = (HFONT)SendMessageW(hwnd_, WM_GETFONT, 0, 0);

    // Get a handle for the DC of the window, and select it into DC
    HDC hdc = GetDC(hwnd_);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Get the text info and text metrics
    SIZE stringSzInfo{ 0 };
    TEXTMETRICW textMetrics{ 0 };
    GetTextExtentPoint32W(hdc, buffer, (int)wcsnlen_s(buffer,MAX_PATH), &stringSzInfo);
    GetTextMetricsW(hdc, &textMetrics);

    // Return the font to the DC, and release the DC
    hFont = (HFONT)SelectObject(hdc, oldFont);
    ReleaseDC(hwnd_, hdc);

    // Calculate the new width and height
    if (lytOpt_.height == nullVal)
    {
      heightCache_ = stringSzInfo.cy + 2 * borderV;
      if (lytOpt_.pad != nullVal) heightCache_ += 2 * lytOpt_.pad;
    }
    else
      heightCache_ = lytOpt_.height;

    if (lytOpt_.width == nullVal)
    {
      widthCache_ = stringSzInfo.cx + 2 * borderH;
      if (lytOpt_.pad != nullVal) widthCache_ += 2 * lytOpt_.pad;
    }
    else
      widthCache_ = lytOpt_.width;

    if (!isStatic)
    {
      RECT r{ 0 };
      GetWindowRect(hwnd_, &r);
      if ((r.right - r.left) > widthCache_)
        widthCache_ = r.right - r.left;
      if ((r.bottom - r.top) > heightCache_)
        heightCache_ = r.bottom - r.top;
    }

    if (lytOpt_.margin != nullVal)
    {
      heightCache_ += 2 * lytOpt_.margin;
      widthCache_ += 2 * lytOpt_.margin;
    }
  }

  void SingleControlLayout::hide()
  {
    hidden_ = true;
    EnableWindow(hwnd_, FALSE);
    ShowWindow(hwnd_, SW_HIDE);
  }

  void SingleControlLayout::show()
  {
    hidden_ = false;
    EnableWindow(hwnd_, TRUE);
    ShowWindow(hwnd_, TRUE);
  }

  void SingleControlLayout::layout(Coord x, Coord y, Coord width, Coord height) 
  {
    if (heightCache_ == nullVal || widthCache_ == nullVal) refreshCache();

    int w, h, xf, yf; // The final calculated position within the space provided

    // Get the width and height - potential for clipping!
    w = widthCache_ < width ? widthCache_ : width;
    h = heightCache_ < height ? heightCache_ : height;

    if (expOpt_ == Expand::Both || expOpt_ == Expand::Horizontal) w = width;
    if (expOpt_ == Expand::Both || expOpt_ == Expand::Vertical) h = height;

    // Handle alignment
    xf = x;
    if (lytOpt_.margin != nullVal) xf += lytOpt_.margin;
    if (w < width)
    {
      switch (hAlign_)
      {
      //case HorizontalAlignment::Left: do nothing, xf already = x
      case HorizontalAlignment::Center:
        xf = x + (width - w) / 2;
        break;
      case HorizontalAlignment::Right:
        xf = x + width - w;
        break;
      }
    }
    yf = y;
    if (lytOpt_.margin != nullVal) yf += lytOpt_.margin;
    if (h < height)
    {
      switch (vAlign_)
      {
        // case VerticalAlignment::Top: do nothing, yf already = y
      case VerticalAlignment::Center:
        yf = y + (height - h) / 2;
        break;
      case VerticalAlignment::Bottom:
        yf = y + height - h;
        break;
      }
    }

    // Finally, shrink actual window size to account for margins.
    if (lytOpt_.margin != nullVal)
    {
      w -= 2 * lytOpt_.margin;
      h -= 2 * lytOpt_.margin;
    }

    MoveWindow(hwnd_, xf, yf, w, h, TRUE);
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, LytOpt lytOpt,
    Expand exOpt, Collapse clpsOpt)
  {
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, exOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Expand expOpt, Collapse clpsOpt)
  {
    LytOpt lytOpt{nullVal, nullVal, padding, nullVal};
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Collapse clpsOpt)
  {
    LytOpt lytOpt{ nullVal, nullVal, padding, nullVal };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height, 
    Expand expOpt, Collapse clpsOpt)
  {
    LytOpt lytOpt{ width, height, nullVal, nullVal };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height,
    Collapse clpsOpt)
  {
    LytOpt lytOpt{ width, height, nullVal, nullVal };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Collapse clpsOpt)
  {
    LytOpt lytOpt{ nullVal, nullVal, nullVal, nullVal };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Expand expOpt, Collapse clpsOpt)
  {
    LytOpt lytOpt{ nullVal, nullVal, nullVal, nullVal };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  void FlowLayout::add(LayoutPtr lyt)
  {
    lyts_.push_back(move(lyt));
    widthCache_ = nullVal;
    heightCache_ = nullVal;
  }

  void FlowLayout::refreshCache()
  {
    // Refresh cached values for components too
    for (auto& lyt : lyts_) lyt->refreshCache();

    widthCache_ = 0;
    heightCache_ = 0;

    if (flowDir_ == Direction::Left || flowDir_ == Direction::Right)
    {
     
      for (auto& lyt : lyts_)
      {
        if (lyt->requestHeight() > heightCache_) 
          heightCache_ = lyt->requestHeight();
        widthCache_ += lyt->requestWidth();
        if (lytOpt_.pad != nullVal) widthCache_ += lytOpt_.pad;
      }
    }
    else
    {
      for (auto& lyt : lyts_)
      {
        if (lyt->requestWidth() > widthCache_)
          widthCache_ = lyt->requestWidth();
        heightCache_ += lyt->requestHeight();
        if (lytOpt_.pad != nullVal) heightCache_ += lytOpt_.pad;
      }
    }

    // Add control margin, remove extra padding
    if (lytOpt_.pad != nullVal)
    {
      if (flowDir_ == Direction::Left || flowDir_ == Direction::Right)
      {
        widthCache_ -= lytOpt_.pad;
      }
      else
        heightCache_ -= lytOpt_.pad;
    }
    if (lytOpt_.margin != nullVal)
    {
      heightCache_ += 2 * lytOpt_.margin;
      widthCache_ += 2 * lytOpt_.margin;
    }
  }

  void FlowLayout::hide()
  {
    for (auto& lyt : lyts_) lyt->hide();
    this->hidden_ = true;
  }

  void FlowLayout::show()
  {
    for (auto& lyt : lyts_) lyt->show();
    this->hidden_ = false;
  }

  void FlowLayout::layout(Coord x, Coord y, Coord width, Coord height)
  {
    if (heightCache_ == nullVal || widthCache_ == nullVal) refreshCache();

    // What direction is the flow, vertical or horizontal
    bool isHorizontal = flowDir_ == Direction::Left || flowDir_ == Direction::Right;

    // Calculate expansion factor for the flow
    Coord extraSize = isHorizontal ? (width - widthCache_) : (height - heightCache_);
    Coord expandableControlsTotalSize = 0;
    vector<double> expandFactor;
    for (auto& lyt : lyts_)
    {
      if (isHorizontal)
      {
        if (lyt->willExpandHorizontal()) expandableControlsTotalSize += lyt->requestWidth();
      }
      else
      {
        if (lyt->willExpandVertical()) expandableControlsTotalSize += lyt->requestHeight();
      }
    }
    for (auto& lyt : lyts_)
    {
      if (isHorizontal)
      {
        if (lyt->willExpandHorizontal()) 
          expandFactor.push_back(static_cast<double>(lyt->requestWidth()) / expandableControlsTotalSize);
        else expandFactor.push_back(0.0);
      }
      else
      {
        if (lyt->willExpandVertical())
          expandFactor.push_back(static_cast<double>(lyt->requestHeight()) / expandableControlsTotalSize);
        else expandFactor.push_back(0.0);
      }
    }

    Coord currX = x;
    Coord currY = y;
    
    switch (flowDir_) 
    {
    case Direction::Right:
    {
      currX += width;
      Coord h = height;
      if (lytOpt_.margin != nullVal) 
      { 
        currX -= lytOpt_.margin;
        currY += lytOpt_.margin;
        h -= 2 * lytOpt_.margin;
      }

      for(size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord w = lyt->requestWidth() + static_cast<Coord>(extraSize * expandFactor[i]);
        currX -= w;
        lyt->layout(currX, currY, w, h);
        if (lytOpt_.pad != nullVal) currX -= lytOpt_.pad;
      }
    }
      break;
    case Direction::Left:
    {
      Coord h = height;
      if (lytOpt_.margin != nullVal)
      {
        currX += lytOpt_.margin;
        currY += lytOpt_.margin;
        h -= 2 * lytOpt_.margin;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord w = lyt->requestWidth() + static_cast<Coord>(extraSize * expandFactor[i]); 
        lyt->layout(currX, currY, w, h);
        currX += w;
        if (lytOpt_.pad != nullVal) currX += lytOpt_.pad;
      }
    }
      break;
    case Direction::Top:
    {
      Coord w = width;
      if (lytOpt_.margin != nullVal)
      {
        currX += lytOpt_.margin;
        currY += lytOpt_.margin;
        w -= 2 * lytOpt_.margin;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord h = lyt->requestHeight() + static_cast<Coord>(extraSize * expandFactor[i]); 
        lyt->layout(currX, currY, w, h);
        currY += h;
        if (lytOpt_.pad != nullVal) currY += lytOpt_.pad;
      }
    }
      break;
    case Direction::Bottom:
    {
      currY += height;
      Coord w = width;
      if (lytOpt_.margin != nullVal)
      {
        currX += lytOpt_.margin;
        currY -= lytOpt_.margin;
        w -= 2 * lytOpt_.margin;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord h = lyt->requestHeight() + static_cast<Coord>(extraSize * expandFactor[i]); 
        currY -= h;
        lyt->layout(currX, currY, w, h);
        if (lytOpt_.pad != nullVal) currY -= lytOpt_.pad;
      }
    }
      break;
    }
  }

  bool FlowLayout::willExpandVertical()
  {
    for (auto& lyt : lyts_) if (lyt->willExpandVertical()) return true;
    return false;
  }

  bool FlowLayout::willExpandHorizontal()
  {
    for (auto& lyt : lyts_) if (lyt->willExpandHorizontal()) return true;
    return false;
  }

  void FlowLayout::resetCache()
  {
    AbstractLayout::resetCache();
    for (auto lyt : lyts_) lyt->resetCache();
  }

  FLayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, Collapse clpsOpt, LytOpt lytOpt)
  {
    return move(FLayoutPtr(new FlowLayout(flowFrom, clpsOpt, lytOpt)));
  }

  FLayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, LytOpt lytOpt)
  {
    return move(FLayoutPtr(new FlowLayout(flowFrom, Collapse::No, lytOpt)));
  }

  FlowLayout::FlowLayout(Direction dir, Collapse clpsOpt, LytOpt lytOpt) :
      AbstractLayout(Expand::No, lytOpt, clpsOpt), flowDir_{ dir }, lyts_()
  {}

  GridLayout::GridLayout(Coord rows, Coord columns, Expand exOpt, LytOpt lytOpt,
    Collapse clpsOpt) : 
      AbstractLayout(exOpt, lytOpt, clpsOpt), 
    lyts_(rows * columns, nullptr), spans_(rows * columns, pair<Coord,Coord>(1,1)), 
    rowHeight_(rows,0), colWidth_(columns,0), numRows_{ rows }, numCols_{columns}
  {}

  void GridLayout::set(Coord row, Coord col, LayoutPtr lyt, Coord rowSpan, Coord colSpan)
  {
    const size_t pos = row * numCols_ + col;
    lyts_[pos] = lyt;
    spans_[pos] = pair<Coord, Coord>(rowSpan, colSpan);

    // Zero out other members of row spans_
    for (Coord r = row + 1; r < row + rowSpan; r++)
    {
      for (Coord c = col + 1; c < col + colSpan; c++)
      {
        const size_t pos2 = r * numCols_ + c;
        spans_[pos2] = pair<Coord, Coord>(0, 0);
      }
    }

    // Reset the cached sizes so they will be recalculated when needed.
    widthCache_ = heightCache_ = nullVal;
  }

  GridLayout::~GridLayout() {}

  void GridLayout::refreshCache()
  {
    for (auto& lyt : lyts_) if (lyt) lyt->refreshCache();

    // Ignore padding, just use margins
    heightCache_ = 0;
    widthCache_ = 0;

    // Reset the cached row/col sizes
    for (auto it = rowHeight_.begin(); it != rowHeight_.end(); ++it) *it = 0;
    for (auto it = colWidth_.begin(); it != colWidth_.end(); ++it) *it = 0;

    // iterate over all the rows/columns, get their height/width
    for (Coord r = 0; r < numRows_; ++r)
      for (Coord c = 0; c < numCols_; ++c)
      {
        auto pos = r * numCols_ + c;
        if (lyts_[pos])
        {
          if (spans_[pos].first == 1 && lyts_[pos]->requestHeight() > rowHeight_[r])
            rowHeight_[r] = lyts_[pos]->requestHeight();
          if (spans_[pos].second == 1 && lyts_[pos]->requestWidth() > colWidth_[c])
            colWidth_[c] = lyts_[pos]->requestWidth();
        }
      }

    // Handle multiple spanning rows/cols
    for (Coord r = 0; r < numRows_; ++r)
      for (Coord c = 0; c < numCols_; ++c)
      {
        auto pos = r * numCols_ + c;
        if (lyts_[pos])
        {
          if (spans_[pos].first > 1) // Spans multiple rows
          {
            const Coord spanRows = spans_[pos].first;
            Coord extraH = lyts_[pos]->requestHeight();
            for (Coord rr = r; rr < r + spanRows; ++rr) extraH -= rowHeight_[rr];
            // Add extra height to last row
            if (extraH > 0) rowHeight_[r + spanRows - 1] += extraH;
          }
          if (spans_[pos].second > 1)
          {
            const Coord spanCols = spans_[pos].second;
            Coord extraW = lyts_[pos]->requestWidth();
            for (Coord cc = c; cc < c + spanCols; ++cc) extraW -= colWidth_[cc];
            // Add extra width to last col
            if (extraW > 0) colWidth_[c + spanCols - 1] += extraW;
          }
        }
      }

    for (auto it = rowHeight_.begin(); it != rowHeight_.end(); ++it) heightCache_ += *it;
    for (auto it = colWidth_.begin(); it != colWidth_.end(); ++it) widthCache_ += *it;

    // Handle margins
    if (lytOpt_.margin != nullVal)
    {
      heightCache_ += lytOpt_.margin;
      widthCache_ += lytOpt_.margin;
    }
  }

  void GridLayout::hide()
  {
    for (auto& lyt : lyts_) lyt->hide();
    this->hidden_ = true;
  }

  void GridLayout::show()
  {
    for (auto& lyt : lyts_) lyt->show();
    this->hidden_ = false;
  }

  void GridLayout::layout(Coord x, Coord y, Coord width, Coord height)
  {
    // Check the cache for a refresh
    if (heightCache_ == nullVal || widthCache_ == nullVal) refreshCache();

    bool expandHorizontal = willExpandHorizontal() || expOpt_ == Expand::Both || expOpt_ == Expand::Horizontal;
    bool expandVertical = willExpandVertical() || expOpt_ == Expand::Both || expOpt_ == Expand::Vertical;

    // Copy calculated, or provided, values
    vector<Coord> finalWidth(colWidth_);
    vector<Coord> finalHeight(rowHeight_);

    // Extra space to expand into, or to use alignment info for
    Coord extraHeight = height - heightCache_;
    Coord extraWidth = width - widthCache_;

    // If expanding, calculate new column widths
    if (expandHorizontal)
    {
      Coord expandableWidth = 0;
      vector<bool> expandCol(numCols_, false);

      // Calculate total requested width of expandable columns for proportional expansion
      for (size_t c = 0; c < numCols_; c++)
      {
        for (size_t r = 0; r < numRows_; r++)
        {
          auto pos = r * numCols_ + c;
          if (lyts_[pos] && lyts_[pos]->willExpandHorizontal())
          {
            expandableWidth += finalWidth[c];
            expandCol[c] = true;
            break;
          }
        }
      }
      // Now expand those columns
      for (size_t c = 0; c < numCols_; c++)
      {
        if (expandCol[c])
        {
          double ratio = static_cast<double>(colWidth_[c]) / expandableWidth;
          finalWidth[c] += static_cast<int>((ratio * extraWidth));
        }
      }
    }

    // If expanding, calculate new row heights
    if (expandVertical)
    {
      vector<bool> expandRow(numRows_, false);
      Coord expandableHeight = 0;

      // Calculate total requested height of expandable rows for proportional expansion
      for (size_t r = 0; r < numRows_; r++)
      {
        for (size_t c = 0; c < numCols_; c++)
        {
          auto pos = r * numCols_ + c;
          if (lyts_[pos] && lyts_[pos]->willExpandVertical())
          {
            expandableHeight += finalHeight[r];
            expandRow[r] = true;
            break;
          }
        }
      }

      // Now extend those rows
      for (size_t r = 0; r < numRows_; r++)
      {
        if (expandRow[r])
        {
          double ratio = static_cast<double>(rowHeight_[r]) / expandableHeight;
          finalHeight[r] += static_cast<int>((ratio * extraHeight));
        }
      }
    }

    // Using alignment information, calculate the upper left corner of the layout
    Coord startX = x, startY = y;
    if (lytOpt_.margin != nullVal)
    {
      startX += lytOpt_.margin;
      startY += lytOpt_.margin;
    }
    // if expanding, we will fill it, otherwise re-align
    if (!expandHorizontal)
    {
      switch (hAlign_)
      {
      //case HorizontalAlignment::Left: // Do nothing, startX already = x
      case HorizontalAlignment::Center:
        startX += extraWidth / 2;
        break;
      case HorizontalAlignment::Right:
        startX += extraWidth;
        break;
      }
    }
    if (!expandVertical)
    {
      switch (vAlign_)
      {
        //case VerticalAlignment::Top: // Do nothing, startY already = y
      case VerticalAlignment::Center:
        startY += extraHeight / 2;
        break;
      case VerticalAlignment::Bottom:
        startY += extraHeight;
        break;
      }
    }

    // Calculate the upper left corner of each cell, and call layout on each item.
    Coord currY = startY;
    for (size_t r = 0; r < numRows_; r++)
    {
      // Always start on the left
      Coord currX = startX;
      for (size_t c = 0; c < numCols_; c++)
      {
        auto pos = r * numCols_ + c;
        if (lyts_[pos])
        {
          auto calcWidth = finalWidth[c];
          auto calcHeight = finalHeight[r];
          // Increase height for row span
          for (size_t rr = r + 1; rr < r + spans_[pos].first; ++rr)
            calcHeight += finalHeight[rr];
          // Increase width for col span
          for (size_t cc = c + 1; cc < c + spans_[pos].second; ++cc)
            calcWidth += finalWidth[cc];
          
          lyts_[pos]->layout(currX, currY, calcWidth, calcHeight);
        }

        // Move to the next column
        currX += finalWidth[c];
      }
      // Move to the next row
      currY += finalHeight[r];
    }
  }

  void GridLayout::resetCache()
  {
    AbstractLayout::resetCache();
    for (auto& lyt: lyts_) if (lyt) lyt->resetCache();
  }

  bool GridLayout::willExpandHorizontal()
  {
    for (auto& lyt : lyts_) 
    { 
      if (lyt && lyt->willExpandHorizontal()) return true; 
    }
    return false;
  }

  bool GridLayout::willExpandVertical()
  {
    for (auto& lyt : lyts_)
    {
      if (lyt && lyt->willExpandVertical()) return true;
    }
    return false;
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Expand exOpt, 
    Collapse clpsOpt, LytOpt lytOpt)
  {
    return move(GLayoutPtr(new GridLayout(rows, columns, exOpt, lytOpt, clpsOpt)));
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Collapse clpsOpt)
  {
    LytOpt lytOpt{nullVal, nullVal, nullVal, nullVal};
    return move(GLayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt, clpsOpt)));
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, LytOpt lytOpt)
  {
    return move(GLayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt,
      Collapse::No)));
  }

}
