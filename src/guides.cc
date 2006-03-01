/********** guides.cc ****************/

/*! \class Guide
 * \brief Things snap to guides.
 *
 * Guides in most programs are either vertical or horizontal lines. Guides in Laidout
 * can be any arbitrary path. Guide objects are used in a viewer to snap objects to them,
 * and they are also used as tabstops and margin indicators in text objects.
 * 
 * A guide can lock its rotation or its scale to be the same relative to the screen. The
 * guide's origin is always transformed with the view space.
 */
/*! \var unsigned int Guide::guidetype
 * \brief The type of guide.
 *
 * <pre>
 *  (1<<0) Keep same angle on screen
 *  (1<<1) Keep same scale on screen
 *  (1<<2) Repeat the path after endpoints, if any
 *  (1<<3) Extend flat lines with tangent at endpoints if any.
 *  (1<<4) attract objects horizontally
 *  (1<<5) attract objects vertically
 *  (1<<6) attract objects perpendicularly
 * </pre>
 */
class Guide : public Laxkit::SomeData
{
 public:
	unsigned int guidetype;
};
