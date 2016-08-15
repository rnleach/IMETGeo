#pragma once
/**
* Utilities for working with the Windows API in namespace Win32Helper.
*
* Layouts.hpp provides utilities for automatically arranging controls in a GUI.
*
* A general note on layouts: They do not respond dynamically to changes in the underlying windows
* controls. E.g. if you call SetText on a static control, the next time the layout manager is run
* it will not recalculate the requested size. There may be a change in the future to enable
* limited recalculations of sizes.
*/
#include <limits>
#include <memory>
#include <vector>

#include <Windows.h>

namespace Win32Helper
{
  using namespace std;

  /// Alias to use for coordinate type. 
  using Coord = int;

  /// undefined or uninitialize value flag.
  // Goofy parenthesis to avoid max MACRO in Windows.h
  static const Coord undefinedCoord = (numeric_limits<Coord>::max)();

  enum class Expand
  {
    No,         // Use requested size only
    Both,       // Grow to fill area
    Horizontal, // Grow horizontally, but not vertically to fill area.
    Vertical,   // Grow vertically, but not horizontially to fill area.
  };

  /// Collapse options are used when hiding controls, when they are hidden, should the area they
  /// occupy be set to zero via height and width request values?
  enum class Collapse
  {
    No,   // Do not collapse, just leave a blank space there
    Yes,  // Yes collapse.
  };

  enum class HorizontalAlignment
  {
    Right,
    Left,
    Center,
  };

  enum class VerticalAlignment
  {
    Top, 
    Bottom,
    Center,
  };

  /**
  * Layout classes will calculate automatically sizes, but if you want to override certain aspects
  * of the calculation, you can with this struct. Default initialize it with 0 to get default
  * behavior.
  *
  * Overriding will be necessary for controls like Edit, ListView, and TreeView controls, where you
  * cannot systematically determine a good width or height, it depends on the use. Whereas for 
  * buttons and static controls, a default width and height can usually be calculated the window
  * text.
  */
  struct LayoutOptions
  {
    Coord overrideWidth;
    Coord overrideHeight;
    Coord overridePadding;
    Coord overrideMargins;
  };

  /**
  * Interface for layouts. Some layout classes will hold a single window, others will compose a 
  * group of sub-layouts. This is the interface layouts will use to communicate with sub-layouts.
  *
  * The protected cache variables are initialized to undefinedCoord to serve as a flag that these
  * have not yet been calculated, and so need to be.
  */
  class AbstractLayout
  {
  public:
    AbstractLayout(Expand expOpt, LayoutOptions lytOpt, Collapse clpsOpt);
    virtual ~AbstractLayout(){};

    /**
    * Minimum requested height and width. Using values less than these in the layout method may
    * cause clipping (most likely) or undefined behavior.
    */
    Coord requestHeight();
    Coord requestWidth();

    /// Query whether this will expand.
    virtual bool willExpandVertical();
    virtual bool willExpandHorizontal();

    /**
    * Height from top of conrol to text baseline. Used when aligning text in static controls with
    * edit labels and check buttons. If there is no text, just return 0.
    */
    virtual Coord baselineHeight();

    /**
    * Hide(show) the control, this will disable(enable) the control and hide(show) it. It uses
    * the value of clpsOpt_ to potentially resize the control to 0 so it does not affect the 
    * layout.
    */
    virtual void hide() = 0;
    virtual void show() = 0;

    /**
    * Set the layout options. Containers should not call these on their sub-containers, because that
    * would override the behavior specified by a programmer. These are here so the programmer can
    * specify their own overrides on a case by case basis.
    */
    inline void setLayoutOptions(LayoutOptions lytOpt) { lytOpt_ = lytOpt; }
    inline void set(Expand expOpt) { expOpt_ = expOpt; }
    inline void set(Collapse clps) { clpsOpt_ = clps; }
    inline void set(HorizontalAlignment hAlign) { hAlign_ = hAlign; }
    inline void set(VerticalAlignment vAlign) { vAlign_ = vAlign; }


    /// Given a starting position and overall size, do the layout.
    virtual void layout(Coord x, Coord y, Coord width, Coord height) = 0;

    /// Force a refresh of the cached values for size. This may be useful if you are dynamically
    /// changing the content of controls.
    virtual void refreshCache() = 0;

  protected:
    LayoutOptions lytOpt_;
    Expand expOpt_;
    Collapse clpsOpt_;
    HorizontalAlignment hAlign_;
    VerticalAlignment vAlign_;
    bool hidden_;
    Coord heightCache_;
    Coord widthCache_;
    Coord baselineCache_;
  };

  /**
  * Manage layouts via shared pointers.
  */
  using LayoutPtr = shared_ptr<AbstractLayout>;

  /**
  * Layout to hold a single control. This is necessary so we can create general layouts that are
  * composed of polymorphic AbstractLayouts, some layouts holding multiple other controls while 
  * others only holding a single control.
  *
  * The margin in the LayoutOptions for a single control layout is ignored. Margins are applied
  * to all layouts in a multiple layout container the same.
  *
  * If a width or height is supplied, then the padding is ignored for that calculation.
  */
  class SingleControlLayout;
  using SCLayoutPtr = shared_ptr<SingleControlLayout>;
  class SingleControlLayout final: public AbstractLayout
  {
  public:
    
    ~SingleControlLayout() override;

    /// Overridden methods of AbsractLayout
    void refreshCache() final;
    void hide() final;
    void show() final;
    void layout(Coord x, Coord y, Coord width, Coord height) final;

    /// Static factory methods
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord },
      Expand exOpt = Expand::No,
      Collapse clpsOpt = Collapse::No);
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Coord padding,
      Expand expOpt = Expand::No,
      Collapse clpsOpt = Collapse::No);
    /// Defaults to NoExpand
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Coord padding,
      Collapse clpsOpt);
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Coord width,
      Coord height,
      Expand expOpt = Expand::No,
      Collapse clpsOpt = Collapse::No);
    /// Defaults to NoExpand
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Coord width,
      Coord height,
      Collapse clpsOpt);
    /// Defaults to NoExpand
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Collapse clpsOpt);
    static SCLayoutPtr makeSingleCtrlLayout(
      HWND control,
      Expand expOpt,
      Collapse clpsOpt = Collapse::No);

    // Private constructor, use static factory methods
    SingleControlLayout(HWND control,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord },
      Expand exOpt = Expand::No,
      Collapse clpsOpt = Collapse::No
    );

  private:

    // Populate the protected cache values for the base class.
    HWND hwnd_;

    // Using the handle, get the control class and calculate the baseline from
    // the control type and window styles.
    void calcBaseline(const int borderV, const SIZE strSz, const TEXTMETRICW& textMetrics);
  };

  /**
  * Flow layout. Just keep adding sub-layouts and they are added horizontally or vertically.
  *
  *
  * The flowDir argument always specifies the direction components will flow from.
  */
  class FlowLayout;
  using FLayoutPtr = shared_ptr<FlowLayout>;
  class FlowLayout final: public AbstractLayout
  {
  public:
    enum class Direction {Top, Bottom, Left, Right };

    ~FlowLayout() override {};

    /// Add a layout to this container
    void add(LayoutPtr lyt);

    /// Dynamcially set the flow direction? Could be annoying to someone.
    inline void set(Direction flowDir){ flowDir_ = flowDir; }

    /// Overridden methods of AbsractLayout
    void refreshCache() final;
    void hide() final;
    void show() final;
    void layout(Coord x, Coord y, Coord width, Coord height) final;
    bool willExpandVertical() final;
    bool willExpandHorizontal() final;

    /// Factory methods
    static FLayoutPtr makeFlowLyt(
      Direction flowFrom,
      Collapse clpsOpt = Collapse::No,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord });

    static FLayoutPtr makeFlowLyt(Direction flowFrom, LayoutOptions lytOpt);

    // Private constructor, call with static factory method.
    FlowLayout(
      Direction dir = Direction::Left,
      Collapse clpsOpt = Collapse::No,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord }
    );

  private:

    Direction flowDir_;              // Layout vertically or horizontally
    vector<LayoutPtr> lyts_;         // Store my layouts here!
  };

  /**
  * GridLayout. Arrange controls in a grid.
  */
  class GridLayout;
  using GLayoutPtr = shared_ptr<GridLayout>;
  class GridLayout final: public AbstractLayout
  {
  public:
    // Set a container to a specific grid cell location
    void set(Coord row, Coord col, LayoutPtr lyt, Coord rowSpan = 1, Coord colSpan = 1);
    using AbstractLayout::set;

    ~GridLayout() override;

    /// Overridden methods of AbsractLayout
    void refreshCache() final;
    void hide() final;
    void show() final;
    void layout(Coord x, Coord y, Coord width, Coord height) final;

    /// Static factory functions
    static GLayoutPtr makeGridLyt(
      Coord rows,
      Coord columns,
      Expand exOpt = Expand::No,
      Collapse clpsOpt = Collapse::No,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord });

    static GLayoutPtr makeGridLyt(
      Coord rows,
      Coord columns,
      Collapse clpsOpt);

    static GLayoutPtr makeGridLyt(
      Coord rows,
      Coord columns,
      LayoutOptions lytOpt);
    
    GridLayout(
      Coord rows,
      Coord columns,
      Expand exOpt = Expand::No,
      LayoutOptions lytOpt = { undefinedCoord, undefinedCoord, undefinedCoord, undefinedCoord },
      Collapse clpsOpt = Collapse::No);

  private:

    // Store layout information in parallel vectors.
    vector<LayoutPtr> lyts_;
    vector<pair<Coord,Coord>> spans_;
    vector<Coord> rowHeight_;
    vector<Coord> colWidth_;
    const size_t numRows_;
    const size_t numCols_;
  };
}
