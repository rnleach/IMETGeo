#include "Layouts.hpp"

#include <iostream>

namespace Win32Helper
{
  AbstractLayout::AbstractLayout(Expand expOpt, LayoutOptions lytOpt, Collapse clpsOpt) :
    expOpt_{ expOpt }, lytOpt_(lytOpt), clpsOpt_{ clpsOpt }, hAlign_{HorizontalAlignment::Center}, 
    vAlign_{VerticalAlignment::Center}, hidden_ { false }, 
    heightCache_{ undefinedCoord }, widthCache_{ undefinedCoord }, baselineCache_{ undefinedCoord }
  {}

  Coord AbstractLayout::requestHeight()
  {
    if (hidden_ && clpsOpt_ == Collapse::Yes) return 0;
    if (heightCache_ == undefinedCoord) refreshCache();
    return heightCache_;
  }

  Coord AbstractLayout::requestWidth()
  {
    if (hidden_ && clpsOpt_ == Collapse::Yes) return 0;
    if (widthCache_ == undefinedCoord) refreshCache();
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

  Coord AbstractLayout::baselineHeight()
  {
    if (hidden_ && clpsOpt_ == Collapse::Yes) return 0;
    if (baselineCache_ == undefinedCoord) refreshCache();
    return baselineCache_;
  }

  SingleControlLayout::SingleControlLayout(HWND control, LayoutOptions lytOpt,
    Expand exOpt, Collapse clpsOpt) :
    AbstractLayout(exOpt, lytOpt, clpsOpt), hwnd_{ control } {}

  SingleControlLayout::~SingleControlLayout() {}

  void SingleControlLayout::refreshCache()
  {
    // Get the window style information
    DWORD dwxStyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);

    // Use window style information to choose message to send to get border sizes
    int borderV = (dwxStyle && WS_EX_CLIENTEDGE) ? SM_CYEDGE : SM_CYBORDER;
    int borderH = (dwxStyle && WS_EX_CLIENTEDGE) ? SM_CXEDGE : SM_CXBORDER;
    borderV = GetSystemMetrics(borderV);
    borderH = GetSystemMetrics(borderH);

    // Get the text in the control
    WCHAR buffer[MAX_PATH];
    GetWindowTextW(hwnd_, buffer, MAX_PATH);

    // Get the font
    HFONT hFont = (HFONT)SendMessageW(hwnd_, WM_GETFONT, 0, 0);

    // Get a handle for the DC of the window, and select it into DC
    HDC hdc = GetDC(hwnd_);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Get the text info and text metrics
    SIZE stringSzInfo{ 0 };
    TEXTMETRICW textMetrics{ 0 };
    GetTextExtentPoint32W(hdc, buffer, wcsnlen_s(buffer,MAX_PATH), &stringSzInfo);
    GetTextMetricsW(hdc, &textMetrics);

    // Return the font to the DC, and release the DC
    hFont = (HFONT)SelectObject(hdc, oldFont);
    ReleaseDC(hwnd_, hdc);

    // Calculate the new width, height, baseline
    if (lytOpt_.overrideHeight == undefinedCoord)
    {
      heightCache_ = stringSzInfo.cy + 2 * borderV;
      if (lytOpt_.overridePadding != undefinedCoord) heightCache_ += 2 * lytOpt_.overridePadding;
      if (lytOpt_.overrideMargins != undefinedCoord) heightCache_ += 2 * lytOpt_.overrideMargins;
    }
    else
      heightCache_ = lytOpt_.overrideHeight;

    if (lytOpt_.overrideWidth == undefinedCoord)
    {
      widthCache_ = stringSzInfo.cx + 2 * borderH;
      if (lytOpt_.overridePadding != undefinedCoord) widthCache_ += 2 * lytOpt_.overridePadding;
      if (lytOpt_.overrideMargins != undefinedCoord) widthCache_ += 2 * lytOpt_.overrideMargins;
    }
    else
      widthCache_ = lytOpt_.overrideWidth;

    // Now do the baseline calculation
    calcBaseline(borderV, stringSzInfo, textMetrics);
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
    if (heightCache_ == undefinedCoord || widthCache_ == undefinedCoord) refreshCache();

    int w, h, xf, yf; // The final calculated position within the space provided

    // Get the width and height - potential for clipping!
    w = widthCache_ < width ? widthCache_ : width;
    h = heightCache_ < height ? heightCache_ : height;


    // Handle expanding
    if ((expOpt_ == Expand::Both || expOpt_ == Expand::Horizontal) && width > w)
      w = width;
    if ((expOpt_ == Expand::Both || expOpt_ == Expand::Vertical) && height > h)
      h = height;

    // Handle alignment
    xf = x;
    if (lytOpt_.overrideMargins != undefinedCoord) xf += lytOpt_.overrideMargins;
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
    if (lytOpt_.overrideMargins != undefinedCoord) yf += lytOpt_.overrideMargins;
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
    if (lytOpt_.overrideMargins != undefinedCoord)
    {
      w -= 2 * lytOpt_.overrideMargins;
      h -= 2 * lytOpt_.overrideMargins;
    }

    MoveWindow(hwnd_, xf, yf, w, h, TRUE);
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, LayoutOptions lytOpt,
    Expand exOpt, Collapse clpsOpt)
  {
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, exOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{undefinedCoord, undefinedCoord, padding, undefinedCoord};
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, padding, undefinedCoord };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height, 
    Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ width, height, undefinedCoord, undefinedCoord };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height,
    Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ width, height, undefinedCoord, undefinedCoord };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  SCLayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord };
    return move(SCLayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  void SingleControlLayout::calcBaseline(const int borderV, const SIZE strSz,
    const TEXTMETRICW & textMetrics)
  {
    // May have to add some testing in here for edit controls, since they don't automaically 
    // center the text in their views like buttons do.
    // May also have to do some checks for static controls, could get hairy here.

    // TODO
    baselineCache_ = borderV + strSz.cy - textMetrics.tmDescent;
    if (lytOpt_.overridePadding != undefinedCoord) baselineCache_ += lytOpt_.overridePadding;
    if (lytOpt_.overrideMargins != undefinedCoord) baselineCache_ += lytOpt_.overrideMargins;
  }

  void FlowLayout::add(LayoutPtr lyt)
  {
    lyts_.push_back(move(lyt));
    widthCache_ = undefinedCoord;
    heightCache_ = undefinedCoord;
  }

  void FlowLayout::refreshCache()
  {
    baselineCache_ = undefinedCoord; // Leave it if vertical flow.
    widthCache_ = 0;
    heightCache_ = 0;

    if (flowDir_ == Direction::Left || flowDir_ == Direction::Right)
    {
      baselineCache_ = 0;
      for (auto& lyt : lyts_)
      {
        if (lyt->baselineHeight() != undefinedCoord && lyt->baselineHeight() > baselineCache_)
          baselineCache_ = lyt->baselineHeight();
      }

      for (auto& lyt : lyts_)
      {
        Coord baselineOffset = 0;
        if (lyt->baselineHeight() < baselineCache_) baselineOffset = baselineCache_ - lyt->baselineHeight();
        if (lyt->requestHeight() + baselineOffset > heightCache_) 
          heightCache_ = lyt->requestHeight() + baselineOffset;
        widthCache_ += lyt->requestWidth();
      }
    }
    else
    {
      for (auto& lyt : lyts_)
      {
        if (lyt->requestWidth() > widthCache_)
          widthCache_ = lyt->requestWidth();
        heightCache_ += lyt->requestHeight();
      }
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
    if (heightCache_ == undefinedCoord || widthCache_ == undefinedCoord) refreshCache();

    // What direction is the flow, vertical or horizontal
    bool isHorizontal = flowDir_ == Direction::Left || flowDir_ == Direction::Right;

    // Calculate expansion factor for the flow
    Coord extraSize =isHorizontal ? (width - widthCache_) : (height - heightCache_);
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
      if (lytOpt_.overrideMargins != undefinedCoord) 
      { 
        currX -= lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for(size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord w = lyt->requestWidth() + static_cast<Coord>(extraSize * expandFactor[i]);
        currX -= w;
        lyt->layout(currX, currY, w, height);
      }
    }
      break;
    case Direction::Left:
    {
      if (lytOpt_.overrideMargins != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord w = lyt->requestWidth() + static_cast<Coord>(extraSize * expandFactor[i]);
        lyt->layout(currX, currY, w, height);
        currX += w;
      }
    }
      break;
    case Direction::Top:
    {
      if (lytOpt_.overrideMargins != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord h = lyt->requestHeight() + static_cast<Coord>(extraSize * expandFactor[i]);
        lyt->layout(currX, currY, width, h);
        currY += h;
      }
    }
      break;
    case Direction::Bottom:
    {
      currY += height;
      if (lytOpt_.overrideMargins != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY -= lytOpt_.overrideMargins;
      }

      for (size_t i = 0; i < lyts_.size(); ++i)
      {
        auto& lyt = lyts_[i];

        Coord h = lyt->requestHeight() + static_cast<Coord>(extraSize * expandFactor[i]);
        currY -= h;
        lyt->layout(currX, currY, width, h);
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

  FLayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, Collapse clpsOpt, LayoutOptions lytOpt)
  {
    return move(FLayoutPtr(new FlowLayout(flowFrom, clpsOpt, lytOpt)));
  }

  FLayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, LayoutOptions lytOpt)
  {
    return move(FLayoutPtr(new FlowLayout(flowFrom, Collapse::No, lytOpt)));
  }

  FlowLayout::FlowLayout(Direction dir, Collapse clpsOpt, LayoutOptions lytOpt) :
      AbstractLayout(Expand::No, lytOpt, clpsOpt), flowDir_{ dir }, lyts_()
  {}

  GridLayout::GridLayout(Coord rows, Coord columns, Expand exOpt, LayoutOptions lytOpt,
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
    baselineCache_ = widthCache_ = heightCache_ = undefinedCoord;
  }

  GridLayout::~GridLayout() {}

  void GridLayout::refreshCache()
  {
    heightCache_ = 0;
    widthCache_ = 0;
    baselineCache_ = undefinedCoord;// Leave it, baseline makes no sense for a grid layout.

    // Reset the cached row/col sizes
    for (auto it = rowHeight_.begin(); it != rowHeight_.end(); ++it) *it = 0;
    for (auto it = colWidth_.begin(); it != colWidth_.end(); ++it) *it = 0;

    // iterate over all the rows/columns, get their height/width
    for (size_t r = 0; r < numRows_; ++r)
      for (size_t c = 0; c < numCols_; ++c)
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
    for (size_t r = 0; r < numRows_; ++r)
      for (size_t c = 0; c < numCols_; ++c)
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
    if (lytOpt_.overrideMargins != undefinedCoord)
    {
      heightCache_ += lytOpt_.overrideMargins;
      widthCache_ += lytOpt_.overrideMargins;
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
    // TODO better column row expansion, only grow columns/rows with expandable controls?

    if (heightCache_ == undefinedCoord || widthCache_ == undefinedCoord) refreshCache();

    // Copy calculated, or provided, values
    vector<Coord> finalWidth(colWidth_);
    vector<Coord> finalHeight(rowHeight_);

    Coord finalTotalWidth = widthCache_, finalTotalHeight = heightCache_;
    if (lytOpt_.overrideMargins != undefinedCoord)
    {
      finalTotalWidth -= lytOpt_.overrideMargins;
      finalTotalHeight -= lytOpt_.overrideMargins;
    }

    // if Expand, proportionally grow/shrink each row/column to fill width and height
    if (expOpt_ == Expand::Both || expOpt_ == Expand::Horizontal)
    {
      auto extraWidth = width - widthCache_;
      if (lytOpt_.overrideMargins != undefinedCoord) extraWidth -= lytOpt_.overrideMargins;
      for (size_t i = 0; i < numCols_; i++)
      {
        double ratio = static_cast<double>(colWidth_[i]) / widthCache_;
        finalWidth[i] += static_cast<int>((ratio * extraWidth));
      }
      finalTotalWidth = width;
      if (lytOpt_.overrideMargins != undefinedCoord) finalTotalWidth -= lytOpt_.overrideMargins;
    }
    if (expOpt_ == Expand::Both || expOpt_ == Expand::Vertical)
    {
      auto extraHeight = height - heightCache_;
      if (lytOpt_.overrideMargins != undefinedCoord) extraHeight -= lytOpt_.overrideMargins;
      for (size_t i = 0; i < numRows_; i++)
      {
        double ratio = static_cast<double>(rowHeight_[i]) / heightCache_;
        finalHeight[i] += static_cast<int>((ratio * extraHeight));
      }
      finalTotalHeight = height;
      if (lytOpt_.overrideMargins != undefinedCoord) finalTotalHeight -= lytOpt_.overrideMargins;
    }

    // Using alignment information, calculate the upper left corner of the layout
    Coord startX = x, startY = y;
    if (lytOpt_.overrideMargins != undefinedCoord)
    {
      startX += lytOpt_.overrideMargins;
      startY += lytOpt_.overrideMargins;
    }
    if (width != finalTotalWidth)
    {
      auto extraWidth = width - finalTotalWidth;
      if (lytOpt_.overrideMargins != undefinedCoord) extraWidth -= lytOpt_.overrideMargins;
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
    if (height != finalTotalHeight)
    {
      auto extraHeight = height - finalTotalHeight;
      if (lytOpt_.overrideMargins != undefinedCoord) extraHeight -= lytOpt_.overrideMargins;
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
          if (spans_[pos].first > 1) // Increase height for row span
          {
            for (size_t rr = r + 1; rr < r + spans_[pos].first; ++rr)
            {
              auto pos2 = rr * numCols_ + c;
              calcHeight += finalHeight[rr];
            }
          }
          if (spans_[pos].second > 1) // Increase width for col span
          {
            for (size_t cc = c + 1; cc < c + spans_[pos].second; ++cc)
            {
              auto pos2 = r * numCols_ + cc;
              calcWidth += finalWidth[cc];
            }
          }

          lyts_[pos]->layout(currX, currY, calcWidth, calcHeight);
        }

        // Move to the next column
        currX += finalWidth[c];
      }
      // Move to the next row
      currY += finalHeight[r];
    }
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Expand exOpt, 
    Collapse clpsOpt, LayoutOptions lytOpt)
  {
    return move(GLayoutPtr(new GridLayout(rows, columns, exOpt, lytOpt, clpsOpt)));
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord};
    return move(GLayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt, clpsOpt)));
  }

  GLayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, LayoutOptions lytOpt)
  {
    return move(GLayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt,
      Collapse::No)));
  }

}
