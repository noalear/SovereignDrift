template<class T, class I> class DynamicArray {
public:
  DynamicArray(void) {
    p = NULL;
    n = I(0);
    err = &logFile;
  }
  DynamicArray(const char* c) {
    p = NULL;
    n = I(0);
    initialize(c);
    err = &logFile;
  }
  DynamicArray(I i, const char* c, ostream* o =&logFile) {
    p = NULL;
    n = I(0);
    err = o;
    initializeWith(i, c);
  }
  ~DynamicArray() {
    destroy();
  }
  int initializeWith(I i, const char* c, ostream* o =&logFile) {
    destroy();
    err = o;
    strcpy(name, c);
    p = new T[i];
    if (p)
      n = i;
    return !p;
  }
  void _initialize() {
    p = 0;
    n = 0;
    *name = '\0';
  }
  void initialize(const char* c =0) {
    if (c)
      strcpy(name, c);

    destroy();
  }
  void destroy() {
    if (p) {

      if (!n)
	*err << "WARNING> attempting to delete a pointer allocated outside of class." << endl;

      delete[] p;
      p = 0;
      n = 0;

    }
  }
  DynamicArray<T,I>& operator=(DynamicArray<T,I>& a) {
    p = a.p;
    n = a.n;
    return *this;
  }
  //DynamicArray& operator=(T* t) {
  //  if (p)
  //    destroy();
  //  
  //  p = t;
  //  n = 0;
  //  err = &logFile;
  //  return *this;
  //}
  I extendBy(I i) {
    T* oldP = p;
    p = new T[n+i];
    
    if (!p) {
      *err << "ERROR> unable to expand \"" << name << "\" array by " << i << " at line " << __LINE__ << endl;
      return 0;
    }

    short j;
    for (j=0; j<n; j++) {
      p[j] = oldP[j];
    }

    n += i;

    if ( oldP )
       delete[] oldP;

    *err << "NOTE> extended \"" << name << "\" by " << i << " to " << n << endl;

    return i;

  }
  I add(T& t) {
    if (extendBy(1)) {
      p[n-1] = t;
      return 1;
    } else
      return 0;
  }
  T& operator[](I i) const {
    /************************************************************/
    /* the return value may be used as an lvalue so it should   */
    /* be safeguarded in the event of a null pointer by return- */
    /* ing a static variable address.  In such an event error   */
    /* message will be the only indication that a problem oc-   */
    /* curred.                                                  */
    /************************************************************/

#ifdef DUMB_ARRAY
    return p[i];
#else
    static T a;
    
    if (p) {
      if (i >= 0 && i < n) {
	return p[i];
      } else {
	*err << "ERROR> illegal attempt to access member " <<
	  i+1 << " out of " << n << " in array \"" << name << "\" at line " << __LINE__ << endl;
	return p[0];
      }
    } else {
      *err << "ERROR> illegal attempt to access member " <<
	i+1 << " from null array." << "in array \"" << name << "\" at line " << __LINE__ << endl;
      return a;
    }
#endif
  }
  T* operator+(I i) {

#ifdef DUMB_ARRAY
    return p + i;
#else
    static T a;

    if (p) {
      if (i >= 0 && i < n) {
	return p + i;
      } else {
	*err << "ERROR> illegal attempt to access member " <<
	  i+1 << " out of " << n << " in array \"" << name << "\" at line " << __LINE__ << endl;
	return p;
      }
    } else {
      *err << "ERROR> illegal attempt to access " << name << " member " <<
	i+1 << " from null array, \"" << name << "\" at line " << __LINE__ << endl;
      return &a;
    }
#endif
  }
  int IsNotNull() {
    return p != 0;
  }
  T* rootOf() {
    return p;
  }
  I indexOf(T* t) {
    return t - p;
  }
  char* nameOf() {
    return name;
  }
private:
  T* p;
  I  n;
  ostream* err;
  char name[64];
};

template<class T, class I, const I size> class StaticArray {
public:
  StaticArray(void) {
    n = size;
    err = &logFile;
  }
  StaticArray(const char* c) {
    n = size;
    initialize(c);
    err = &logFile;
  }
  ~StaticArray() {
    destroy();
  }
  void initialize(const char* c =0) {
    if (c)
      strcpy(name, c);

    destroy();
  }
  void destroy() {
    /* do nothing */
  }
  T& operator[](I i) const {
    /************************************************************/
    /* the return value may be used as an lvalue so it should   */
    /* be safeguarded in the event of a null pointer by return- */
    /* ing a static variable address.  In such an event error   */
    /* message will be the only indication that a problem oc-   */
    /* curred.                                                  */
    /************************************************************/
    T* tp = const_cast<T*> (p);

#ifdef DUMB_ARRAY
    return tp[i];
#else
    static T a;
    
    if (p) {
      if (i >= 0 && i < n) {
	return tp[i];
      } else {
	*err << "ERROR> illegal attempt to access member " <<
	  i+1 << " out of " << n << " in array \"" << name << "\" at line " << __LINE__ << endl;
	return tp[0];
      }
    } else {
      *err << "ERROR> illegal attempt to access member " <<
	i+1 << " from null array." << "in array \"" << name << "\" at line " << __LINE__ << endl;
      return a;
    }
#endif
  }
  T* operator+(I i) {

#ifdef DUMB_ARRAY
    return p + i;
#else
    static T a;
    T* tp = const_cast<T*> (p);

    if (p) {
      if (i >= 0 && i < n) {
	return tp + i;
      } else {
	*err << "ERROR> illegal attempt to access member " <<
	  i+1 << " out of " << n << " in array \"" << name << "\" at line " << __LINE__ << endl;
	return tp;
      }
    } else {
      *err << "ERROR> illegal attempt to access " << name << " member " <<
	i+1 << " from null array, \"" << name << "\" at line " << __LINE__ << endl;
      return &a;
    }
#endif
  }
  int IsNotNull() {
    return p != 0;
  }
  T* rootOf() {
    return p;
  }
  I indexOf(T* t) {
    return t - p;
  }
  char* nameOf() {
    return name;
  }
private:
  T p[size];
  I n;
  ostream* err;
  char name[64];
};

#define ARRAY_INITIALIZE(array_name) array_name.initialize(#array_name)
#define ARRAY_INITIALIZEWITH(array_name, size) array_name.initializeWith(size, #array_name)
