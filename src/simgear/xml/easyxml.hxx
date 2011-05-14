/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       easyxml.hxx
 Author:       David Megginson
 Date started: 2000

 License:      David Megginson has placed easyXML into the public domain.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef __EASYXML_HXX
#define __EASYXML_HXX

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
HEADERS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "../compiler.h"

#include STL_IOSTREAM
#include STL_STRING
#include <vector>

SG_USING_STD(istream);
SG_USING_STD(string);
SG_USING_STD(vector);

/** Interface for XML attributes.
 *
 * This interface is used to provide a list of attributes to the
 * application.  The interface is a pure abstract class so that
 * different implementations can be substituted for the sake of
 * efficiency.
 *
 * @see XMLAttributesDefault
 */

class XMLAttributes
{
public:

  /// Constructor.
  XMLAttributes ();

  /// Destructor.
  virtual ~ XMLAttributes ();

  /** Get the number of attributes present.
   * @return The number of attributes in the list (may be 0).
   */
  virtual int size () const = 0;

  /** Get the name of an attribute by index.
   * The index must be less than the size of the list and greater
   * than or equal to zero.
   * @param i The index of the attribute (zero-based).
   * @see #size
   */
  virtual const char * getName (int i) const = 0;

  /** Get the string value of an attribute by index.
   *
   * The index must be less than the size of the list and greater
   * than or equal to zero.
   *
   * @param i The index of the attribute (zero-based).
   * @see #size
   */
  virtual const char * getValue (int i) const = 0;

  /** Look up the index of an attribute by name.
   *
   * Attribute names must be unique.  This method will return
   * an index that can be used with the <tt>getValue(const char *name)</tt>
   * method if the attribute is found.
   *
   * @param name The name of the attribute.
   * @return The index of the attribute with the name specified,
   * or -1 if no such attribute is present in the list.
   */
  virtual int findAttribute (const char * name) const;

  /** Test whether an attribute is present.
   *
   * @param name The name of the attribute.
   * @return true if an attribute with the specified name is present
   * in the attribute list, false otherwise.
   */
  virtual bool hasAttribute (const char * name) const;

  /** Look up the value of an attribute by name.
   *
   * This method provides a convenient short-cut to invoking
   * {@link #findAttribute} and <tt>getValue(const char *name)</tt>.
   *
   * @param name The name of the attribute to look up.
   * @return The attribute's value as a string, or 0 if no
   * attribute was found with the name specified.
   */
  virtual const char * getValue (const char * name) const;
};


/** Default mutable attributes implementation.
 *
 * This class provides a default implementation of the {@link
 * XMLAttributes} interface.  The implementation is mutable, so
 * that it is possible to modify the attribute list when necessary.
 * This class is particularly useful for taking a snapshot of
 * an attribute list during parsing.
 *
 * @see XMLAttributes
 */

class XMLAttributesDefault : public XMLAttributes
{
public:

  /// Default constructor.
  XMLAttributesDefault ();

  /** Copy constructor.
   *
   * This constructor is especially useful for taking a static
   * snapshot of an attribute list for later use.
   *
   * @param atts The attribute list to copy.
   */
  XMLAttributesDefault (const XMLAttributes & atts);

  /// Destructor.
  virtual ~XMLAttributesDefault ();

  /// Count the attributes in the list.
  virtual int size () const;

  /// Get the name of an attribute by index.
  virtual const char * getName (int i) const;

  /// Get the value of an attribute by index.
  virtual const char * getValue (int i) const;

  /** Add an attribute to an attribute list.
   *
   * The name is required to be unique in the list; the value is not.
   *
   * @param name The name of the attribute to add.
   * @param value The value of the attribute to add.
   */
  virtual void addAttribute (const char * name, const char * value);

  /** Set an attribute name by index.
   *
   * This method will not extend the list; the attribute must
   * already exist.
   *
   * @param i The index of the attribute (zero-based).
   * @param name The new name.
   */
  virtual void setName (int i, const char * name);

  /** Set an attribute value by index.
   *
   * This method will not extend the list; the attribute must
   * already exist.
   *
   * @param i The index of the attribute (zero-based).
   * @param value The new value.
   */
  virtual void setValue (int i, const char * value);

  /** Set an attribute value by name.
   *
   * This method will not extend the list; the attribute must
   * already exist.
   *
   * @param name The name of the attribute that will have the new
   * value.
   * @param value The new value.
   */
  virtual void setValue (const char * name, const char * value);

private:
  vector<string> _atts;
};


/** Visitor class for an XML document.
 *
 * This interface uses the Visitor pattern.  The XML parser walks
 * through the XML document and invokes the appropriate method in
 * this visitor for each piece of markup it finds.  By default,
 * the methods do nothing; the application must subclass the visitor
 * and override the methods for the events it's interested in.
 * All applications are required to provide an implementation
 * for the error callback.
 */

class XMLVisitor
{
public:

  /// Virtual destructor.
  virtual ~XMLVisitor () {}

  /** Callback for the start of an XML document.
   *
   * The XML parser will invoke this method once, at the beginning of
   * the XML document, before any other methods are invoked.  The
   * application can use this callback to set up data structures,
   * open files, etc.
   *
   * @see #endXML
   */
  virtual void startXML () {}

  /** Callback for the end of an XML document.
   *
   * The XML parser will invoke this method once, at the end of the
   * XML document, after all other methods are invoked, and only
   * if there have been no parsing errors.  The application can use
   * this callback to close or write files, finalize data structures,
   * and so on, but the application will need to be prepared to
   * clean up any resources without this callback in the event of
   * an error.
   *
   * @see #startXML
   */
  virtual void endXML () {}

  /** Callback for the start of an XML element.
   *
   * The XML parser will invoke this method at the beginning of every
   * XML element.  Start and end element calls will be balanced
   * and properly nested: every element has both a start and end
   * callback (even if it was specified with an XML empty element tag),
   * there is exactly one root element, and every element must end
   * before its parent does.  Elements may not overlap.
   * Note that the attribute list provided is volatile; it's contents
   * are not guaranteed to persist after the end of the callback.
   * If the application needs to keep a copy of the attribute list,
   * it can make the copy with the {@link XMLAttributesDefault} class.
   *
   * @param name The name of the element that is starting (not null).
   * @param atts The element's attributes (not null).
   * @see #endElement
   */
  virtual void startElement (const char * name, const XMLAttributes &atts) {}

  /** Callback for the end of an XML element.
   *
   * The XML parser will invoke this method at the end of every XML element.
   *
   * @param name The name of the element that is ending (not null).
   * @see #startElement
   */
  virtual void endElement (const char * name) {}

  /** Callback for a chunk of character data.
   *
   * The XML parser will invoke this method once for every chunk of
   * character data in the XML document, including whitespace
   * separating elements (as required by the XML recommendation).
   * Note that character data may be chunked arbitrarily: the
   * character data content of an element may be returned in one
   * large chunk or several consecutive smaller chunks.
   *
   * @param s A pointer to the beginning of the character data (not null).
   * @param length The number of characters in the chunk (may
   * be zero).
   */
  virtual void data (const char * s, int length) {}

  /** Callback for an XML processing instruction.
   *
   * The XML parser will invoke this method once for every processing
   * instruction in the XML document.  Note that the XML declaration
   * and the Text declaration are NOT PROCESSING INSTRUCTIONS and
   * will not be reported through this callback.  Processing
   * instructions are not all that useful, but the XML recommendation
   * requires that they be reported.  Most applications can safely
   * ignore this callback and use the empty default implementation.
   *
   * @param target The processing instruction target (not null).
   * @param data The processing instruction data (not null).
   */
  virtual void pi (const char * target, const char * data) {}

  /** Callback for an XML parsing warning.
   *
   * The XML parser will use this callback to report any non-fatal warnings
   * during parsing.  It is the responsibility of the application to
   * deal with the warning in some appropriate way.
   *
   * @param message The warning message from the parser.
   * @param line The number of the line that generated the warning.
   * @param column The character position in the line that generated
   * the warning.
   */
  virtual void warning (const char * message, int line, int column) {}
};

/** @relates XMLVisitor
 * Read an XML document.
 *
 * This function reads an XML document from the input stream provided,
 * and invokes the callback methods in the visitor object to pass the
 * parsing events back to the application.  When this function
 * returns, the parser will have reported all of the data in the XML
 * document to the application through the visitor callback methods,
 * and XML processing will be complete.
 *
 * @param input The byte input stream containing the XML document.
 * @param visitor An object that contains callbacks for XML parsing
 * events.
 * @param path A string describing the original path of the resource.
 * @exception Throws sg_io_exception or sg_xml_exception if there
 * is a problem reading the file.
 * @see XMLVisitor
 */

extern void readXML (istream &input, XMLVisitor &visitor, const string &path="");

/** @relates XMLVisitor
 * Read an XML document.
 *
 * This function reads an XML document from the input stream provided,
 * and invokes the callback methods in the visitor object to pass the
 * parsing events back to the application.  When this function
 * returns, the parser will have reported all of the data in the XML
 * document to the application through the visitor callback methods,
 * and XML processing will be complete.
 *
 * @param path The file name of the XML resource.
 * @param visitor An object that contains callbacks for XML parsing
 * events.
 * @exception Throws sg_io_exception or sg_xml_exception if there
 * is a problem reading the file.
 * @see XMLVisitor
 */
extern void readXML (const string &path, XMLVisitor &visitor);

#endif // __EASYXML_HXX
