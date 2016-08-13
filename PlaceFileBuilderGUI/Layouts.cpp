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

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, LayoutOptions lytOpt,
    Expand exOpt, Collapse clpsOpt)
  {
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, exOpt, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{undefinedCoord, undefinedCoord, padding, undefinedCoord};
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord padding, 
    Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, padding, undefinedCoord };
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height, 
    Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ width, height, undefinedCoord, undefinedCoord };
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Coord width, Coord height,
    Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ width, height, undefinedCoord, undefinedCoord };
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord };
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, Expand::No, clpsOpt)));
  }

  LayoutPtr SingleControlLayout::makeSingleCtrlLayout(HWND control, Expand expOpt, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{ undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord };
    return move(LayoutPtr(new SingleControlLayout(control, lytOpt, expOpt, clpsOpt)));
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
    Coord currX = x;
    Coord currY = y;

    switch (flowDir_) 
    {
    case Direction::Right:
    {
      currX += width;
      if (lytOpt_.overrideHeight != undefinedCoord) 
      { 
        currX -= lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for (auto& lyt : lyts_)
      {
        Coord w = lyt->requestWidth();
        currX -= w;
        lyt->layout(currX, currY, w, height);
      }
    }
      break;
    case Direction::Left:
    {
      if (lytOpt_.overrideHeight != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for (auto& lyt : lyts_)
      {
        Coord w = lyt->requestWidth();
        lyt->layout(currX, currY, w, height);
        currX += w;
      }
    }
      break;
    case Direction::Top:
    {
      if (lytOpt_.overrideHeight != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY += lytOpt_.overrideMargins;
      }

      for (auto& lyt : lyts_)
      {
        Coord h = lyt->requestHeight();
        lyt->layout(currX, currY, width, h);
        currY += h;
      }
    }
      break;
    case Direction::Bottom:
    {
      currY += height;
      if (lytOpt_.overrideHeight != undefinedCoord)
      {
        currX += lytOpt_.overrideMargins;
        currY -= lytOpt_.overrideMargins;
      }

      for (auto& lyt : lyts_)
      {
        Coord h = lyt->requestHeight();
        currY -= h;
        lyt->layout(currX, currY, width, h);
      }
    }
      break;
    }
  }

  LayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, Collapse clpsOpt, LayoutOptions lytOpt)
  {
    return move(LayoutPtr(new FlowLayout(flowFrom, clpsOpt, lytOpt)));
  }

  LayoutPtr FlowLayout::makeFlowLyt(Direction flowFrom, LayoutOptions lytOpt)
  {
    return move(LayoutPtr(new FlowLayout(flowFrom, Collapse::No, lytOpt)));
  }

  FlowLayout::FlowLayout(Direction dir, Collapse clpsOpt, LayoutOptions lytOpt) :
      AbstractLayout(Expand::No, lytOpt, clpsOpt), flowDir_{ dir }, lyts_()
  {}

  GridLayout::GridLayout(Coord rows, Coord columns, Expand exOpt, LayoutOptions lytOpt,
    Collapse clpsOpt) : 
      AbstractLayout(exOpt, lytOpt, clpsOpt), 
    lyts_(rows * columns, nullptr), rowHeight_(rows,0), colWidth_(columns,0),
    numRows_{ rows }, numCols_{columns}
  {}

  void GridLayout::set(Coord row, Coord col, LayoutPtr lyt)
  {
    const size_t pos = row * numCols_ + col;
    lyts_[pos] = lyt;

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
          if (lyts_[pos]->requestHeight() > rowHeight_[r])
            rowHeight_[r] = lyts_[pos]->requestHeight();
          if (lyts_[pos]->requestWidth() > colWidth_[c])
            colWidth_[c] = lyts_[pos]->requestWidth();
        }
      }

    for (auto it = rowHeight_.begin(); it != rowHeight_.end(); ++it) heightCache_ += *it;
    for (auto it = colWidth_.begin(); it != colWidth_.end(); ++it) widthCache_ += *it;
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
    if (heightCache_ == undefinedCoord || widthCache_ == undefinedCoord) refreshCache();

    // Copy calculated, or provided, values
    vector<Coord> finalWidth(colWidth_);
    vector<Coord> finalHeight(rowHeight_);
    Coord finalTotalWidth = widthCache_, finalTotalHeight = heightCache_;

    // if Expand, proportionally grow/shrink each row/column to fill width and height
    if (expOpt_ == Expand::Both || expOpt_ == Expand::Horizontal)
    {
      auto extraWidth = width - widthCache_;
      for (size_t i = 0; i < numCols_; i++)
      {
        double ratio = static_cast<double>(colWidth_[i]) / widthCache_;
        finalWidth[i] += static_cast<int>((ratio * extraWidth));
      }
      finalTotalWidth = width;
    }
    if (expOpt_ == Expand::Both || expOpt_ == Expand::Vertical)
    {
      auto extraHeight = height - heightCache_;
      for (size_t i = 0; i < numRows_; i++)
      {
        double ratio = static_cast<double>(rowHeight_[i]) / heightCache_;
        finalHeight[i] += static_cast<int>((ratio * extraHeight));
      }
      finalTotalHeight = height;
    }

    // Using alignment information, calculate the upper left corner of the layout
    Coord startX = x, startY = y;
    if (width != finalTotalWidth)
    {
      auto extraWidth = width - finalTotalWidth;
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
        if(lyts_[pos]) lyts_[pos]->layout(currX, currY, finalWidth[c], finalHeight[r]);

        // Move to the next column
        currX += finalWidth[c];
      }
      // Move to the next row
      currY += finalHeight[r];
    }
  }

  LayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Expand exOpt, 
    Collapse clpsOpt, LayoutOptions lytOpt)
  {
    return move(LayoutPtr(new GridLayout(rows, columns, exOpt, lytOpt, clpsOpt)));
  }

  LayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, Collapse clpsOpt)
  {
    LayoutOptions lytOpt{undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord};
    return move(LayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt, clpsOpt)));
  }

  LayoutPtr GridLayout::makeGridLyt(Coord rows, Coord columns, LayoutOptions lytOpt)
  {
    return move(LayoutPtr(new GridLayout(rows, columns, Expand::No, lytOpt,
      Collapse::No)));
  }
}
