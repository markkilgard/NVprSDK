
/*
 * Copyright 2010 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkPDFTypes_DEFINED
#define SkPDFTypes_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"
#include "SkString.h"
#include "SkTDArray.h"
#include "SkTypes.h"

class SkPDFCatalog;
class SkWStream;

/** \class SkPDFObject

    A PDF Object is the base class for primitive elements in a PDF file.  A
    common subtype is used to ease the use of indirect object references,
    which are common in the PDF format.
*/
class SkPDFObject : public SkRefCnt {
public:
    /** Create a PDF object.
     */
    SkPDFObject();
    virtual ~SkPDFObject();

    /** Return the size (number of bytes) of this object in the final output
     *  file. Compound objects or objects that are computationally intensive
     *  to output should override this method.
     *  @param catalog  The object catalog to use.
     *  @param indirect If true, output an object identifier with the object.
     */
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    /** For non-primitive objects (i.e. objects defined outside this file),
     *  this method will add to resourceList any objects that this method
     *  depends on.  This operates recursively so if this object depends on
     *  another object and that object depends on two more, all three objects
     *  will be added.
     *  @param resourceList  The list to append dependant resources to.
     */
    virtual void getResources(SkTDArray<SkPDFObject*>* resourceList);

    /** Emit this object unless the catalog has a substitute object, in which
     *  case emit that.
     *  @see emitObject
     */
    void emit(SkWStream* stream, SkPDFCatalog* catalog, bool indirect);

    /** Helper function to output an indirect object.
     *  @param catalog The object catalog to use.
     *  @param stream  The writable output stream to send the output to.
     */
    void emitIndirectObject(SkWStream* stream, SkPDFCatalog* catalog);

    /** Helper function to find the size of an indirect object.
     *  @param catalog The object catalog to use.
     */
    size_t getIndirectOutputSize(SkPDFCatalog* catalog);

    /** Static helper function to add a resource to a list.  The list takes
     *  a reference.
     * @param resource  The resource to add.
     * @param list      The list to add the resource to.
     */
    static void AddResourceHelper(SkPDFObject* resource,
                                  SkTDArray<SkPDFObject*>* list);

    /** Static helper function to copy and reference the resources (and all
     *   their subresources) into a new list.
     * @param resources The resource list.
     * @param result    The list to add to.
     */
    static void GetResourcesHelper(SkTDArray<SkPDFObject*>* resources,
                                   SkTDArray<SkPDFObject*>* result);

protected:
    /** Subclasses must implement this method to print the object to the
     *  PDF file.
     *  @param catalog  The object catalog to use.
     *  @param indirect If true, output an object identifier with the object.
     *  @param stream   The writable output stream to send the output to.
     */
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect) = 0;
};

/** \class SkPDFObjRef

    An indirect reference to a PDF object.
*/
class SkPDFObjRef : public SkPDFObject {
public:
    /** Create a reference to an existing SkPDFObject.
     *  @param obj The object to reference.
     */
    explicit SkPDFObjRef(SkPDFObject* obj);
    virtual ~SkPDFObjRef();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    SkRefPtr<SkPDFObject> fObj;
};

/** \class SkPDFInt

    An integer object in a PDF.
*/
class SkPDFInt : public SkPDFObject {
public:
    /** Create a PDF integer (usually for indirect reference purposes).
     *  @param value An integer value between 2^31 - 1 and -2^31.
     */
    explicit SkPDFInt(int32_t value);
    virtual ~SkPDFInt();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    int32_t fValue;
};

/** \class SkPDFBool

    An boolean value in a PDF.
*/
class SkPDFBool : public SkPDFObject {
public:
    /** Create a PDF boolean.
     *  @param value true or false.
     */
    explicit SkPDFBool(bool value);
    virtual ~SkPDFBool();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    bool fValue;
};

/** \class SkPDFScalar

    A real number object in a PDF.
*/
class SkPDFScalar : public SkPDFObject {
public:
    /** Create a PDF real number.
     *  @param value A real value.
     */
    explicit SkPDFScalar(SkScalar value);
    virtual ~SkPDFScalar();

    static void Append(SkScalar value, SkWStream* stream);

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);

private:
    SkScalar fValue;
};

/** \class SkPDFString

    A string object in a PDF.
*/
class SkPDFString : public SkPDFObject {
public:
    /** Create a PDF string. Maximum length (in bytes) is 65,535.
     *  @param value A string value.
     */
    explicit SkPDFString(const char value[]);
    explicit SkPDFString(const SkString& value);

    /** Create a PDF string. Maximum length (in bytes) is 65,535.
     *  @param value     A string value.
     *  @param len       The length of value.
     *  @param wideChars Indicates if the top byte in value is significant and
     *                   should be encoded (true) or not (false).
     */
    SkPDFString(const uint16_t* value, size_t len, bool wideChars);
    virtual ~SkPDFString();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    static SkString FormatString(const char* input, size_t len);
    static SkString FormatString(const uint16_t* input, size_t len,
                                 bool wideChars);
private:
    static const size_t kMaxLen = 65535;

    const SkString fValue;

    static SkString DoFormatString(const void* input, size_t len,
                                 bool wideInput, bool wideOutput);
};

/** \class SkPDFName

    A name object in a PDF.
*/
class SkPDFName : public SkPDFObject {
public:
    /** Create a PDF name object. Maximum length is 127 bytes.
     *  @param value The name.
     */
    explicit SkPDFName(const char name[]);
    explicit SkPDFName(const SkString& name);
    virtual ~SkPDFName();

    bool operator==(const SkPDFName& b) const;

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

private:
    static const size_t kMaxLen = 127;

    const SkString fValue;

    static SkString FormatName(const SkString& input);
};

/** \class SkPDFArray

    An array object in a PDF.
*/
class SkPDFArray : public SkPDFObject {
public:
    /** Create a PDF array. Maximum length is 8191.
     */
    SkPDFArray();
    virtual ~SkPDFArray();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    /** The size of the array.
     */
    int size() { return fValue.count(); }

    /** Preallocate space for the given number of entries.
     *  @param length The number of array slots to preallocate.
     */
    void reserve(int length);

    /** Returns the object at the given offset in the array.
     *  @param index The index into the array to retrieve.
     */
    SkPDFObject* getAt(int index) { return fValue[index]; }

    /** Set the object at the given offset in the array. Ref's value.
     *  @param index The index into the array to set.
     *  @param value The value to add to the array.
     *  @return The value argument is returned.
     */
    SkPDFObject* setAt(int index, SkPDFObject* value);

    /** Append the object to the end of the array and increments its ref count.
     *  @param value The value to add to the array.
     *  @return The value argument is returned.
     */
    SkPDFObject* append(SkPDFObject* value);

    /** Creates a SkPDFInt object and appends it to the array.
     *  @param value The value to add to the array.
     */
    void appendInt(int32_t value);

    /** Creates a SkPDFScalar object and appends it to the array.
     *  @param value The value to add to the array.
     */
    void appendScalar(SkScalar value);

    /** Creates a SkPDFName object and appends it to the array.
     *  @param value The value to add to the array.
     */
    void appendName(const char name[]);

private:
    static const int kMaxLen = 8191;
    SkTDArray<SkPDFObject*> fValue;
};

/** \class SkPDFDict

    A dictionary object in a PDF.
*/
class SkPDFDict : public SkPDFObject {
public:
    /** Create a PDF dictionary. Maximum number of entries is 4095.
     */
    SkPDFDict();

    /** Create a PDF dictionary with a Type entry.
     *  @param type   The value of the Type entry.
     */
    explicit SkPDFDict(const char type[]);

    virtual ~SkPDFDict();

    // The SkPDFObject interface.
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    /** The size of the dictionary.
     */
    int size() { return fValue.count(); }

    /** Add the value to the dictionary with the given key.  Refs value.
     *  @param key   The key for this dictionary entry.
     *  @param value The value for this dictionary entry.
     *  @return The value argument is returned.
     */
    SkPDFObject* insert(SkPDFName* key, SkPDFObject* value);

    /** Add the value to the dictionary with the given key.  Refs value.  The
     *  method will create the SkPDFName object.
     *  @param key   The text of the key for this dictionary entry.
     *  @param value The value for this dictionary entry.
     *  @return The value argument is returned.
     */
    SkPDFObject* insert(const char key[], SkPDFObject* value);

    /** Add the int to the dictionary with the given key.
     *  @param key   The text of the key for this dictionary entry.
     *  @param value The int value for this dictionary entry.
     */
    void insertInt(const char key[], int32_t value);

    /** Add the scalar to the dictionary with the given key.
     *  @param key   The text of the key for this dictionary entry.
     *  @param value The scalar value for this dictionary entry.
     */
    void insertScalar(const char key[], SkScalar value);

    /** Add the name to the dictionary with the given key.
     *  @param key   The text of the key for this dictionary entry.
     *  @param name  The name for this dictionary entry.
     */
    void insertName(const char key[], const char name[]);

    /** Add the name to the dictionary with the given key.
     *  @param key   The text of the key for this dictionary entry.
     *  @param name  The name for this dictionary entry.
     */
    void insertName(const char key[], const SkString& name) {
        this->insertName(key, name.c_str());
    }

    /** Remove all entries from the dictionary.
     */
    void clear();

private:
    struct Rec {
      SkPDFName* key;
      SkPDFObject* value;
    };

public:
    class Iter {
    public:
        explicit Iter(const SkPDFDict& dict);
        SkPDFName* next(SkPDFObject** value);

    private:
        Rec* fIter;
        Rec* fStop;
    };

private:
    static const int kMaxLen = 4095;

    SkTDArray<struct Rec> fValue;
};

#endif
