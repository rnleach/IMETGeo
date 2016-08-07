/**
* Utilities for working with the Windows API in namespace Win32Helper.
*
* Layouts.hpp provides utilities for automatically arranging controls in a GUI.
*/
#include <memory>
#include <vector>

namesapce Win32Helper
{
  enum class ExpandOptions
  {
    NoExpand;         // Use requested size only
    Expand;           // Grow to fill area
    ExpandHorizontal; // Grow horizontally, but not vertically to fill area.
    ExpandVertical;   // Grow vertically, but not horizontially to fill area.
  };

  enum class HorizontalAlignment
  {
    Right;
    Left;
    Center;
  };

  enum class VerticalAlignment
  {
    Top; 
    Bottom;
    Center;
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
    uint overrideWidth;
    uint overrideHeight;
    uint overridePadding;
    uint overrideMargins;
  };

  /**
  * Interface for layouts. Some layout classes will hold a single window, others will compose a 
  * group of sub-layouts. This is the interface layouts will use to communicate with sub-layouts.
  */
  class AbstractLayout
  {
  public:
    AbstractLayout(ExpandOptions expOpt, LayoutOptions lytOpt);
    virtual ~AbstractLayout(){};

    /**
    * Minimum requested height and width. Using values less than these in the layout method may
    * cause clipping (most likely) or undefined behavior.
    */
    virtual uint requestHeight() const = 0;
    virtual uint requestWidth() const = 0;

    /**
    * Height from top of conrol to text baseline. Used when aligning text in static controls with
    * edit labels and check buttons. If there is no text, just return 0.
    */
    virtual uint baselineHeight() const = 0;

    /**
    * Set the layout options. Containers should not call these on their sub-containers, because that
    * would override the behavior specified by a programmer. These are here so the programmer can
    * specify their own overrides on a case by case basis.
    */
    void setLayoutOptions(LayoutOptions lytOpt);
    void setExpandOption(ExpandOptions expOpt);


    /// Given a starting position and overall size, do the layout.
    virtual void layout(uint x, uint y, uint width, uint height) const = 0;

  protected:
    LayoutOptions lytOpt_;
    ExpandOptions xpOpt_;
  };

  /**
  * Manage layouts via shared pointers.
  */
  using LayoutPtr = shared_ptr<AbstractLayout>;

  /**
  * Layout to hold a single control. This is necessary so we can create general layouts that are
  * composed of polymorphic AbstractLayouts, some layouts holding multiple other controls while 
  * others only holding a single control.
  */
  class SingleControlLayout final: public AbstractLayout
  {
  public:
    explicit SingleContorlLayout(HWND control, ExpandOptions exOpt = ExpandOptions::NoExpand,
      LayoutOptions lytOpt = {0});

    /// Overridden methods of AbsractLayout
    final uint requestHeight() const;
    final uint requestWidth() const;
    final uint baselineHeight() const;
    final void layout(uint x, uint y, uint width, uint height) const;

  private:
    HWND hwnd_;
  };

  /**
  * Flow layout. Just keep adding sub-layouts and they are added horizontally or vertically.
  */
  class FlowLayout final: public AbstractLayout
  {
  public:
    enum class Direction { Vertical; Horizontal; };

    FlowLayout(
      Direction dir              = Direction::Horizontal, 
      HorizontalAlignment hAlign = HorizontalAlignment::Left,
      VerticalAlignment vAlign   = VerticalAlignment::Top,
      ExpandOptions exOpt        = ExpandOptions::NoExpand, 
      LayoutOptions lytOpt       = {0}
    );

    override ~FlowLayout();

    // Add a layout to this container
    void add(LayoutPtr lyt);

    // Not sure if I should have these, should you be able to adjust these after construction?
    // This would give you the ability to dynmaically adjust the GUI layout during run time with
    // no real additional cost. That ability is a little fancier than my original goal for the
    // layouts, but it is just so easy to do, I'll do it for now.
    inline void setHorizontalAlignment(HorizontalAlignment hAlign){ hAlign_ = hAlign; }
    inline void setVerticalAlignment(VerticalAlignment vAlign){ vAlign_ = vAlign; }
    inline void setFlowDirection(Direction flowDir){ flowDir_ = flowDir; }

    /// Overridden methods of AbsractLayout
    final uint requestHeight() const;
    final uint requestWidth() const;
    final uint baselineHeight() const;
    final void layout(uint x, uint y, uint width, uint height) const;

  private:
    HorizontalAlignment hAlign_;  // If not expanding sub-layouts, align them how horizontally?
    VerticalAlignment vAlign_;    // If not expanding sub-layouts, align them how vertically
    Direction flowDir_;           // Layout vertically or horizontally

    vector<LayoutPtr> lyts_;      // Store my layouts here!
  };

  /**
  * GridLayout. Arrange controlls in a grid.
  */
  class GridLayout final: public AbstractLayout
  {
    GridLayout(
      uint rows, 
      uint columns, 
      ExpandOptions exOpt  = ExpandOptions::NoExpand, 
      LayoutOptions lytOpt = {0});

    // Set a container to a specific grid cell location
    void set(
      uint row, 
      uint col, 
      LayoutPtr lyt, 
      HorizontalAlignment hAlign = HorizontalAlignment::Center, 
      VerticalAlignment vAign    = VerticalAlignment::Center
    );

    override ~GridLayout();

    /// Overridden methods of AbsractLayout
    final uint requestHeight() const;
    final uint requestWidth() const;
    final uint baselineHeight() const;
    final void layout(uint x, uint y, uint width, uint height) const;

  private:
    // Store layout information in parallel vectors.
    vector<HorizontalAlignment> hAlign_;
    vector<VerticalAlignment> vAlign_;
    vector<LayoutPtr> lyts_;
  }
}