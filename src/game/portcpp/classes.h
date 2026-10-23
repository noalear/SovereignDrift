/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
enum FileType {generic_file, data_file, html_file};

class SFString {
 public:
  SFString(const SFString&);
  SFString(const char*);
  SFString(const SFString&, const SFString&);
  SFString(const SFString&, const unsigned int, const unsigned int);
  SFString(const int);
  SFString(const double, const SFString);
  SFString();
  ~SFString();
  SFString& operator=(const char*);
  SFString& operator=(const SFString&);
  char& operator[](ulong);
  const char& operator[](ulong) const;
  SFString operator+(const SFString&) const;
  SFString operator+(char) const;
  int operator==(const SFString&) const;
  int operator!=(const SFString&) const;
  friend ostream& operator<<(ostream&, const SFString&);
  friend istream& operator>>(istream&, SFString&); 
  int ReadFrom(istream&);
  int ReadFrom(SFString&);
  unsigned int Index(const SFString&);
  unsigned int Index(const SFString&, int);
  unsigned int count(const SFString&);
  SFString sub(const unsigned int, const unsigned int);
  SFString rem(const SFString&);
  SFString rem(const SFString&, const int);
  SFString rem(const ulong, const ulong);
  SFString ins(const SFString&, const unsigned int);
  SFString rep(const SFString&, const SFString&);
  SFString subs(const SFString&, const SFString&);
  SFString strip(char ='"');
  SFString quote();
  char* out() const { 
    if (p->s) {
      char* t = new char[p->l+1]; strcpy(t,p->s);
      return t;
    } else
      return 0;
  }
  char* addr() const { return p->s; }
  SFString crop();
  double Double() const;
  slong Long() const;
  short Short() const;
  int Int() const;
  int IsNull() const {
    return !p->s || !(*p->s);
  }
  int TokenList( SFString*&,  const SFString&, slong** =0) const;
  int InTokenList(SFString, const SFString) const;
  SFString TokenString(const SFString*, const int, const SFString);
  int NTokens(const SFString&) const;
  SFString WordList(SFString&,  const SFString*, const int, const SFString);
  int match(const SFString&);
  SFString StHexToStInt();
  slong StHexToInt();
  inline friend unsigned int len(const SFString& st) {
    return st.p->l;
  }
 private:
  struct sdata {
    char* s;
    ulong n;
    ulong l;
    sdata() {s = 0; n = 1; l = 0;}
  };
  sdata* p;
};

struct Element {
  class Catalog* list;
  slong           number;
  SFString         str;
};
  
class Catalog {
public:
  enum ElementType {numeric_element, string_element, list_element};
  Catalog() {
    name = 0;
    value = 0;
    etype = 0;
    n = 0;
    nmax = 0;
  }
  Catalog(const Catalog& c) {
    CopyFrom(c);
  }
  Catalog(int nm) {
    name = new SFString[nm];
    value = new Element[nm];
    etype = new ElementType[nm];
    if (name && value && etype)
      nmax = nm;
    n = 0;
  }
  ~Catalog() {
    if (name)
      delete[] name;
    if (value)
      delete[] value;
    if (etype)
      delete etype;
    
    name = 0;
    value = 0;
    etype = 0;

  }
  Catalog& operator=(const Catalog& c) {
    CopyFrom(c);
    return *this;
  }
  void CopyFrom(const Catalog& c) {
    int i;
    name = new SFString[c.n];
    value = new Element[c.n];
    etype = new ElementType[c.n];
    if (name && value && etype) {
      n = nmax = c.n;
      for (i=0; i<n; i++) {
	name[i] = c.name[i];
	value[i].number = c.value[i].number;
	value[i].str = c.value[i].str;
	value[i].list = c.value[i].list;
	etype[i] = c.etype[i];
      }
    }
  }
  int ApplySubs(SFString from, SFString to) {
    SFString val;
    int i;
    int nsubs=0;
    for (i=0; i<n; i++) {
      if (!value[i].str.IsNull()) {
	if (value[i].str.Index(from)) {
	  nsubs++;
	  value[i].str = value[i].str.subs(from, to);
	}
      }
    }
    return nsubs;
  }
  int ApplyFilter(Catalog& filter) {
    short i;
    SFString val;
    for (i=0; i<filter.n; i++) {
      val = LookUpStringValueByName(filter.name[i]);
      if (!val.IsNull())
	if (!val.match(filter.value[i].str))
	  /* failed this test so return a failed condition */
	  return 0;
    }
    /* passed all filter tests */
    return 1;
  }
  void List(SFString prefix, SFString* pattern =0) {
    short i;

    if (prefix.IsNull())
      prefix = SFString("<null>");

    for (i=0; i<n; i++) {

      if (!pattern ||
	  pattern && name[i].match(*pattern)) {
	
	switch(etype[i]) {
	case string_element:
	  cout << prefix << "(" << i << ") " << name[i] << " -> " <<
	    value[i].str << endl;
	  break;
	case numeric_element:
	  cout << prefix << "(" << i << ") " << name[i] << " -> " <<
	    value[i].number << endl;
	  break;
	case list_element:
	  cout << prefix << "(" << i << ") " << name[i] << " -> <list>" <<
	    endl;
	  value[i].list->List(SFString(" ") + prefix + "->" + name[i]);
	  break;
	}
      }

    }
  }
  void TabulateHeader(class DataDocument*, SFString, SFString* =0);
  void Tabulate(class DataDocument*, SFString, SFString* =0);
  int ExpandBy(short nexp) {

    /* try to get more memory */
    SFString*      newName  = new SFString[nmax + nexp];
    ElementType* newEType = new ElementType[nmax + nexp];
    Element*     newValue = new Element[nmax + nexp];

    short i;

    if (newName && newEType && newValue) { /* the request was successful */

      for (i=0; i<n; i++) {
	
	newName[i]  = name[i];
	newValue[i] = value[i];
	newEType[i] = etype[i];

      }

      delete[] name;
      delete[] value;
      delete[] etype;
      name = newName;
      value = newValue;
      etype = newEType;
      nmax += nexp;
      return 1;
    } else /* the allocation failed */
      return -1;

  }
  int Add(SFString s, SFString val, int dupCheck = 1) {

    if (dupCheck && Index(s) != -1)
      return 0;

    if (n < nmax) {
      name[n] = s;
      value[n].str = val;
      etype[n] = string_element;
      n++;
      return 1;
    } else if (ExpandBy(100)) {
      name[n] = s;
      value[n].str = val;
      etype[n] = string_element;
      n++;
      return 1;
    } else
      return 0;
  }
  int Add(SFString s, slong val, int dupCheck = 1) {

    if (dupCheck && Index(s) != -1)
      return 0;

    if (n < nmax) {
      name[n] = s;
      value[n].number = val;
      etype[n] = numeric_element;
      n++;
      return 1;
    } else if (ExpandBy(100)) {
      name[n] = s;
      value[n].number = val;
      etype[n] = numeric_element;
      n++;
      return 1;
    } else
      return 0;
  }
  int Add(SFString s, Catalog* val, int dupCheck = 1) {

    if (dupCheck && Index(s) != -1)
      return 0;

    if (n < nmax) {
      name[n] = s;
      value[n].list = val;
      etype[n] = list_element;
      n++;
      return 1;
    } else if (ExpandBy(100)) {
      name[n] = s;
      value[n].list = val;
      etype[n] = list_element;
      n++;
      return 1;
    } else
      return 0;
  }
  int Index(SFString s, int occur =1) const {
    int i;
    int no = 0;
    for (i=0; i<n; i++)
      if (name[i] == s && ++no == occur)
	return i;
    return -1;
  }
  int Match(SFString p, int occur =1) const {
    int i;
    int no = 0;
    for (i=0; i<n; i++)
      if (name[i].match(p) && ++no == occur)
	return i;
    return -1;
  }
  int NumberOfOccurrences(SFString s) {
    int i,j = 0;
    for (i=0; i<n; i++) {
      if (name[i] == s)
	j++;
    }
    
    return j;
  }
  int NumberOfMatches(SFString p) {
    int i,j = 0;
    for (i=0; i<n; i++) {
      if (name[i].match(p))
	j++;
    }
    
    return j;
  }
  Catalog* CatalogDistinctMatches(SFString p) const {

    /* the local catalog should be deleted by the calling program */

    int i = 0;
    Catalog* local = new Catalog;
    for (i=0; i<n; i++) {
      if (name[i].match(p))
	local->Add(name[i], "", 1);
    }
    return local;
  }
  SFString LookUpStringValueByName(SFString s, int occur =1) const {
    int index = Index(s, occur);
    if (index != -1 && etype[index] == string_element)
      return value[index].str;
    else
      return 0;
  }
  SFString MatchStringValueByName(SFString p, SFString& cname, int occur =1) const {
    int index = Match(p, occur);
    if (index != -1 && etype[index] == string_element) {
      cname = name[index];
      return value[index].str;
    } else
      return 0;
  }
  slong LookUpNumberValueByName(SFString s, int occur =1) const {
    int index = Index(s, occur);
    if (index != -1 && etype[index] == numeric_element)
      return value[index].number;
    else
      return 0;
  }
  slong MatchNumberValueByName(SFString p, SFString& cname, int occur =1) const {
    int index = Match(p, occur);
    if (index != -1 && etype[index] == numeric_element) {
      cname = name[index];
      return value[index].number;
    } else
      return 0;
  }
  Catalog* LookUpListValueByName(SFString s, int occur =1) const {
    int index = Index(s, occur);
    if (index != -1 && etype[index] == list_element)
      return value[index].list;
    else
      return 0;
  }
  Catalog* MatchListValueByName(SFString p, SFString& cname, int occur =1) const {
    int index = Match(p, occur);
    if (index != -1 && etype[index] == list_element) {
      cname = name[index];
      return value[index].list;
    } else
      return 0;
  }
  SFString LookUpNameByStringValue(SFString val) const {
    short i;
    for (i=0; i<n; i++)
      if (value[i].str == val)
	return name[i];

    return SFString("");
  }
  SFString LookUpNameByNumberValue(slong val) const {
    short i;
    for (i=0; i<n; i++)
      if (value[i].number == val)
	return name[i];

    return SFString("");
  }
  int Population() const {
    return n;
  }
  void SetStringValueByName(SFString sn, SFString sv) const {
    int i = Index(sn);
    if (i != -1)
      value[i].str = sv;
  }
  void SetNumberValueByName(SFString sn, slong d) const {
    int i = Index(sn);
    if (i != -1)
      value[i].number = d;
  }
  void SetNameByIndex(int i, SFString s) const {
    name[i] = s;
  }
  void SetStringValueByIndex(int i, SFString s) const {
    value[i].str = s;
  }
  void SetNumberValueByIndex(int i, slong d) const {
    value[i].number = d;
  }
  SFString LookUpNameByIndex(int i) const {
    return name[i];
  }
  SFString LookUpStringValueByIndex(int i) const {
    return value[i].str;
  }
  slong LookUpNumberValueByIndex(int i) const {
    return value[i].number;
  }
  slong IncrementNumberValueByIndex(int i, slong ulimit, slong llimit =0) {

    value[i].number = ((value[i].number + 1) % (ulimit - llimit)) - llimit;

    return value[i].number;
  }
 private:
  SFString*        name;
  Element*       value;
  ElementType*   etype;
  int            n;
  int            nmax;
};

class Document {
public:
  Document() {}
  Document(char* fname) {
    init(SFString(fname));
    OpenDocument();
  }
  Document(SFString fname) {
    init(fname);
    OpenDocument();
  }
  Document(ostream* out) {
    init(out);
    OpenDocument();
  }
  virtual ~Document() {
    CloseDocument();
    if (f && f != &cout)
      delete f;
    f = 0;
  }
  void init(SFString fname) {
    filename = fname;
    init(new ofstream(filename.out()));
  }
  void init(ostream* out) {
    f = out;
  }
  virtual FileType type() const {
    return generic_file;
  }
  virtual void OpenDocument() {
  }    
  virtual void CloseDocument() {
  }
  virtual void OpenHead() {
  }
  virtual void CloseHead() {
  }
  virtual void OpenBody() {
  }
  virtual void CloseBody() {
  }
  virtual void WriteCell(SFString text, SFString* cellFormatText =0) {
  }
  virtual void WriteCell(double, SFString) {
  }
  virtual void WriteCell(slong) {
  }
  virtual void WriteToCell(SFString text) {
  }
  virtual void WriteToCell(double, SFString) {
  }
  virtual void WriteToCell(slong) {
  }
  virtual void OpenRow(SFString* rowText =0) {
  }
  virtual void CloseRow() {
    *f << endl;
  }
  virtual void OpenTable() {}
  virtual void OpenTable(SFString) {
  }
  virtual void CloseTable() {
  }
  virtual void WriteLine(const SFString line) const {
    *f << line << endl;
  }
  /* data */
  ostream* f;
  SFString  filename;
};

class DataDocument : public Document {
public:
  enum align_type {default_alignment, left, right, center};
  DataDocument() : Document() {}
  DataDocument(char* fname) : Document(fname) {}
  DataDocument(SFString fname) : Document(fname) {}
  DataDocument(ostream* out) : Document(out) {}
  virtual ~DataDocument() {
    if (table)
      CloseDocument();
  }
  FileType type() const {
    return data_file;
  }
  void OpenDocument() {
  }    
  void CloseDocument() {
    if (table)
      CloseTable();
  }
  void OpenHead() {
  }
  void CloseHead() {
  }
  void OpenBody() {
  }
  void CloseBody() {
  }
  void WriteCell(double f, SFString fmt) {
    SFString s(f, fmt);
    AddCell(s);
  }
  void WriteCell(slong num) {
    SFString s(num);
    AddCell(s);
  }
  void WriteCell(SFString text, SFString* cellFormatText =0) {
    AddCell(text);
  }
  void WriteToCell(double f, SFString fmt) {
    SFString s(f, fmt);
    AddToCell(s);
  }
  void WriteToCell(slong num) {
    SFString s(num);
    AddToCell(s);
  }
  void WriteToCell(SFString text) {
    AddToCell(text);
  }
  void AddToCell(SFString text) {
      
      table[nrows-1][currentCol] = table[nrows-1][currentCol] + " " +
	text;

  }
  void AddCell(SFString text, int alignment =default_alignment) {
    if (nrows > 1) { /* we have already built the first row */

      /* if the default alignment is selected then use the column default (already set) */
      if (alignment == default_alignment)
	alignment = align[nrows-2][currentCol];
      
      table[nrows-1][currentCol] = text;
      align[nrows-1][currentCol] = alignment;

      currentCol++;

    } else {
      
      SFString* oldRow = 0;
      int* oldAlignRow = 0;

      if (table) {
	oldRow = table[0];
	oldAlignRow = align[0];
      } else { /* create the table with one row */
	table = new SFString*[1];
	align = new int*[1];
      }

      table[0] = new SFString[ncols+1];
      align[0] = new int[ncols+1];

      if (!table[0] || !align[0]) {
	/* error -- unable to allocate */
	table[0] = oldRow;
	align[0] = oldAlignRow;
	return;
      }

      short j;
      for (j=0; j<ncols; j++) {

	table[0][j] = oldRow[j];
	align[0][j] = oldAlignRow[j];

      }

      if (oldRow) {
	delete[] oldRow;
	oldRow = 0;
      }

      if (oldAlignRow) {
	delete oldAlignRow;
	oldAlignRow = 0;
      }

      /* the default alignment for the first row is left */
      if (alignment == default_alignment)
	alignment = left;

      table[0][ncols] = text;
      align[0][ncols] = alignment;

      ncols++;

    }
  }
  void OpenRow(SFString* rowText =0) {

    if (nrows) { /* we have already built the first row */

      currentCol = 0;
     
      SFString** oldTable = table;
      int** oldAlign = align;

      table = new SFString*[nrows+1];
      align = new int*[nrows+1];

      if (!table) {
	table = oldTable;
	return;
      }

      if (!align) {
	align = oldAlign;
	return;
      }

      short i;
      for (i=0; i<nrows; i++) {
	table[i] = oldTable[i];
	align[i] = oldAlign[i];
      }

      table[nrows] = new SFString[ncols];
      align[nrows] = new int[ncols];

      nrows++;

      delete oldTable;
      delete oldAlign;

      oldTable = 0;
      oldAlign = 0;

    } else
      nrows = 1;
  }
  void CloseRow() {
  }
  void OpenTable() {
    /* initialize table data */
    ncols = 0;
    nrows = 0;
    table = 0;
    align = 0;
  }
  void OpenTable(SFString options) {

    OpenTable();

    if (!options.IsNull()) {

      /* options not parsed */

    }

  }
  void CloseTable() {
    WriteTable();

    short i;
    for (i=0; i<nrows; i++) {
      delete[] table[i];
    }

    delete table;
    table = 0;

    delete align;
    align = 0;
  }
  void WriteTable() {
    short i,j;
    short* l = new short[ncols];

    for (j=0; j<ncols; j++)
      l[j] = GetMaxColWidth(j) + 1;

    for (i=0; i<nrows; i++) {
      for (j=0; j<ncols; j++) {
	if (align[i][j] == left) {
	  f->setf(ios::left);
	  f->width(l[j]);
	  *f << table[i][j];
	} else {
	  f->setf(ios::right);
	  f->width(l[j]-1);
	  *f << table[i][j] << " ";
	}
      }
      *f << endl;
    }

    delete l;
    l = 0;
  }
  short GetMaxColWidth(short j) {
    short i;
    unsigned short maxWidth = 0;
    for (i=0; i<nrows; i++)
      maxWidth = max(maxWidth, len(table[i][j]));

    return maxWidth;
  }
  /* data */
  short ncols;
  short nrows;
  short currentCol;
  SFString** table;
  int** align;
};

class HTMLDocument : public Document {
public:
  HTMLDocument() : Document() {}
  HTMLDocument(char* fname) : Document(fname) {}
  HTMLDocument(SFString fname) : Document(fname) {}
  HTMLDocument(ostream* out) : Document(out) {}
  virtual ~HTMLDocument() {}
  FileType type() const {
    return html_file;
  }
  void OpenDocument() {
    *f << "<HTML>" << endl;
  }    
  void CloseDocument() {
    *f << "</HTML>" << endl;
  }
  void OpenHead() {
    *f << "<head>" << endl;
    *f << "<title>Title</title>" << endl;
  }
  void CloseHead() {
    *f << "</head>" << endl;
  }
  void OpenBody() {
    *f << "<body>" << endl;
  }
  void CloseBody() {
    *f << "</body>" << endl;
  }
  void WriteCell(SFString text, SFString* cellFormatText =0) {
    *f << "<TD" ;
    if (cellFormatText && !cellFormatText->IsNull())
      *f << " " << *cellFormatText;
    *f << ">";
    if (!cellText.IsNull())
      *f << cellText;
    *f << " " <<
      text << "</TD>" << endl;
    cellText = "";
  }
  void WriteCell(double d, SFString fmt) {
    SFString s(d, fmt);
    WriteCell(s);
  }
  void WriteCell(slong num) {
    SFString s(num);
    WriteCell(s);
  }
  void WriteToCell(SFString text) {
    if (!cellText.IsNull())
      cellText = cellText + " <BR> " + text;
    else
      cellText = text;
  }
  void WriteToCell(double d, SFString fmt) {
    SFString s(d, fmt);
    WriteToCell(s);
  }
  void WriteToCell(slong num) {
    SFString s(num);
    WriteToCell(s);
  }
  void OpenRow(SFString* rowText =0) {
    *f << "<TR";
    if (rowText && !rowText->IsNull())
      *f << *rowText;
    *f << ">" << endl;
    cellText = "";
  }
  void CloseRow() {
    *f << "</TR>" << endl;
  }
  void OpenTable(SFString options) {
    *f << "<TABLE" << options << ">" << endl;
  }
  void OpenTable() {
    *f << "<TABLE>" << endl;
  }
  void CloseTable() {
    *f << "</TABLE>" << endl;
  }
private:
  SFString cellText;
};
