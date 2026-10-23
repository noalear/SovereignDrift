/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include <S_Lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#ifndef max
   #define max(a,b) ((a)>(b)?(a):(b))
   #define min(a,b) ((a)<(b)?(a):(b))
#endif

#include "classes.h"

// Useful Function Definitions

#ifndef max
#define max(i, j) (i>j?i:j)
#define min(i, j) (i<j?i:j)
#endif
inline int max3(int i, int j, int k) {return max(max(i,j),max(j,k));}
inline int mid(int i, int j, int k) {return max3(min(i,j), min(i,k), min(j,k));}

// Class Definitions

SFString::SFString(const SFString& st)
{
  if ( st.p )
    {
      st.p->n++;   // increment reference counter
    }
  p = st.p;    // copy reference
}


SFString::SFString(const char* c) {
  p = new sdata;            // create new structure
  if (c && p) {
    p->l = strlen(c);         // find and store length
    p->s = new char[p->l+1];  // allocate space for new char string
    strcpy(p->s, c);          // copy string to new space
  }
}

SFString::SFString(const SFString& s1, const SFString& s2) {
  /* concatenation constructor */
  p = new sdata;
  
  if ( p )
    {
      /* calculate space for concatenated strings */
      p->l = s1.p->l + s2.p->l;
      
      /* allocate space for new char* */
      char* s12 = new char[p->l+1]; 
      
      *s12 = '\0';
      
      /* copy and concatenate */
      if (s1.p->s)
	strcpy(s12, s1.p->s);
      if (s2.p->s)
	strcat(s12, s2.p->s);
      
      /* copy reference */
      p->s = s12;
    }
}


SFString::SFString(const SFString& st, const unsigned int i1, const unsigned int i2)
     /* constructor: creates String based on substring of another string */
{
  p = new sdata;
  
  if (p && st.p && i1 > 0 && i2 >= i1 && i1 <= st.p->l) {
    p->l = max(min(i2,st.p->l) - i1 + 1, 0);
    p->s = new char[p->l+1];
    strncpy(p->s, &st.p->s[i1-1], p->l);
    p->s[p->l] = '\0';
  }    
}


SFString::SFString(const int i)
{
  /* initialize string with integer */
  static char c[24];
  sprintf(c, "%d", i);

  p = new sdata;            // create new structure
  if ( p )
    {
      p->l = strlen(c);         // find and store length
      p->s = new char[p->l+1];  // allocate space for new char string
      strcpy(p->s, c);          // copy string to new space
    }
}


SFString::SFString(const double f, const SFString fmt = 0) {
  /* initialize string with floating point number */
  char c[64];

  if (!&fmt)
    sprintf(c, "%f", f);
  else
    sprintf(c, fmt.out(), f);

  p = new sdata;            // create new structure
  if ( p )
    {
      p->l = strlen(c);         // find and store length
      p->s = new char[p->l+1];  // allocate space for new char string
      strcpy(p->s, c);          // copy string to new space
    }

}
  
SFString::SFString()
{
  p = new sdata;    
  if ( p )
    {
      p->s = 0;         // default null string
    }
}


SFString::~SFString()
{
  if ( p )
    {
      if (--p->n == 0)
	{  // if no more outstanding references:
	  if (p->s)
	    delete [] p->s;   //   free char* allocation
	  p->s = 0;         //   set the pointer to null
	  delete p;         //   free string allocation
	}
    }
}


SFString& SFString::operator=(const char* c) {
  if ( p )
    {
      if (p->n > 1) {          // if additional outstanding references to old:
	p->n--;                //   remove self-reference
	p = new sdata;         //   make new string allocation
      } else if (p->s) {       // otherwise
	delete [] p->s;        //   delete char* allocation (keep string)
	p->s = 0;              //   set the pointer to null
      }
      
      if (c)
	p->l = strlen(c);        // calculate and store string length
      else
	p->l = 0;
      
      if (p->l) {
	p->s = new char[p->l+1]; // allocate new char* space
	strcpy(p->s, c);         // copy new string to new space
      }
    }
  return *this;            // return self reference
}

SFString& SFString::operator=(const SFString& st) {
  if ( p && st.p )
    {
      st.p->n++;             // increment st reference count;
      if (--p->n == 0) {     // if no additional outstanding references to old:
	if (p->s) {
	  delete [] p->s;    //   free old char* allocation
	  p->s = 0;          //   set the pointer to null
	}
	delete p;            //   free old string allocation
      }
      
      p = st.p;              // copy new string reference;
    }
  return *this;            // return self-reference
}



char _ch[1] = {0};

char& SFString::operator[](ulong i) {
  if ( p )
    {
      if (i<0 || p->l < i) cerr << "index out of range\n";
      if (p->n > 1) {
	/* duplicate to protect existing references to this string */
	sdata* np = new sdata;
	np->s = new char[p->l+1];
	np->l = p->l;
	strcpy(np->s, p->s);
	p->n--;
	p = np;
      }
      return p->s[i];
    }
  cerr << "NULL pointer\n";
  return _ch[0];
}



const char& SFString::operator[](ulong i) const {
  /* constant subscript */
  if ( p )
    {  
      if (i<0 || p->l < i) cerr << "index out of range\n";
      return p->s[i];
    }
  cerr << "NULL pointer\n";
  return _ch[0];
}


SFString SFString::operator+(const SFString& st) const {
/* return concatenated strings */
  return SFString(*this, st);     // use the concatenation constructor
}

SFString SFString::operator+(char c) const {
/* return concatenated strings */
  return SFString(*this, &c);     // use the concatenation constructor
}
//String::operator char*() const {      // char* conversion
//  char* tCopy = new char[p->l+1];       //   allocate space for copy
//  strcpy(tCopy, p->s);                //   copy
//  return tCopy;                       //   return pointer to copy
//}                                     //  ** copy is persistent **

int SFString::operator==(const SFString& st) const {
  if (p && p->s && st.p->s)
    /* both are non-NULL */
    return (strcmp(p->s,st.p->s) == 0);
  else if (p && !p->s && !st.p->s)
    /* both are NULL strings */
    return 1;
  else
    /* one is non-NULL */
    return 0;
}

int SFString::operator!=(const SFString& st) const {
  if (p && p->s && st.p && st.p->s)
    /* both are non-NULL strings */
    return (strcmp(p->s,st.p->s) != 0);
  else if (p && st.p && (p->s || st.p->s) )
    /* one is a NULL string */
    return 1;
  else
    /* both are NULL strings */
    return 0;
}

ostream& operator<<(ostream& outs, const SFString& st) {
  if ( st.p )
    {
      return outs << st.p->s;               // return append ostream
    }
  return outs;
}

istream& operator>>(istream& ins, SFString& st) {
  const int maxBufferSize = 1024;
  char buffer[maxBufferSize];

  ins >> buffer;

  //ins.get(buffer, maxBufferSize, '\n'); // get buffer

  st = buffer;                           // copy char* buffer to string;

  return ins;
}

int SFString::ReadFrom(istream& is) {
  /* read from an input stream, but do not allow it to interrupt a */
  /* quoted string */

  char c;
  char cs[2];

  cs[1] = '\0';

  short done = 0, quoted = 0, empty = 1;
  

  do {

    if (is.get(c)) {
      *cs = c;
      switch (c) {
      case '\0':
	return 0;
      case '\n':
      case ' ':
	if (!quoted && !empty)
	  done = 1;
	else if (quoted)
	  *this = *this + cs;
	break;
      case '"':
	quoted = 1 - quoted;
	*this = *this + cs;
	empty = 0;
	break;
      default:
    	*this = *this + cs;
	empty = 0;
	break;
      }
    }

  } while (!done);

  return done;

}

int SFString::ReadFrom(SFString& st) {
  /* read from an input string, but do not allow it to interrupt a */
  /* quoted string */
  /* the string will be "consumed" in the same way that an input stream */
  /* would be */

  short done = 0, quoted = 0, empty = 1;
  ulong i = 0;

  /* initialize the current string */
  *this = SFString();

  SFString cs;

  while (!done && !st.IsNull()) {

    i++;

    if (!st.IsNull())
      cs = st.sub(1,1);
    else
      cs = SFString();

    if ( cs.p )
      {
	switch (*cs.p->s) {
	case '\0':
	  return 0;
	case '\n':
	case ' ':
	  if (!quoted && !empty)
	    done = 1;
	  else {
	    if (quoted)
	      *this = *this + cs;
	    st = st.rem(1UL,1UL);
	  }
	  break;
	case '"':
	  quoted = 1 - quoted;
	  *this = *this + cs;
	  st = st.rem(1UL,1UL);
	  empty = 0;
	  break;
	default:
	  *this = *this + cs;
	  st = st.rem(1UL,1UL);
	  empty = 0;
	  break;
	}
      }
  }
    
  return done;

}

unsigned int SFString::Index(const SFString& st) {
  // returns position of matching argument string in target string (self)
  // return value: 1 = first character.

  char* sc = 0;
  if (p && st.p && p->s && st.p->s) {
    sc = strstr(p->s, st.p->s);
    return int(sc-p->s+1 > 0 ? sc-p->s+1 : 0);
  } else
    return 0;
}

unsigned int SFString::Index(const SFString& st, int n) {
  // returns position of [n]th occurrence of matching argument string 
  // in target string (self) (0 if not found)
  // return value: 1 = first character.
  
  int i = 0;
  char* c;
  int k = 0;
  if ( p )
    {
      while ((c = strstr(p->s+i, st.p->s)) && k < n)
	{
	  i = c - p->s + 1; 
	  k++;
	}
    }
  return k == n? i: 0;
}

unsigned int SFString::count(const SFString& st) {
  // returns number of occurences of string st in this string

  int i = 0;
  char* c;
  int k = 0;
  if (p && p->s && st.p && st.p->s) {
    while ((c = strstr(p->s+i, st.p->s)) != 0) {
      i = c - p->s + 1; 
      k++;
    }
  }
  return k;
}

SFString SFString::sub(const unsigned int i, const unsigned int j) {
  /* returns substring [i,j] */

  /* use substring constructor */
  return SFString(*this,i,j);
}

SFString SFString::rem(const SFString& st) {
  // removes the argument string from this string
  // and returns result

  int i = Index(st);
  if ( p )
    {
      if (i == 0) {         // if no match in this:
	return SFString(p->s);    //   return this string
      } else {
	if (i == 1) {            // if match in first character
	  return sub(len(st) + 1,p->l);   //   return remainder of this string
	} else {                // otherwise
	  //                         return modified string
	  return sub(1,i-1) + sub(i+len(st),p->l);
	}
      }
    }
  return *this;
}


SFString SFString::rem(const SFString& st, const int i) {
  // removes the argument string from this string at location [i-1]
  // and returns result

  if ( p )
    {
      if (sub(i, i+len(st)-1) != st) {     // if no match in this:
	return SFString(p->s);              //   return this string
      } else {                            // otherwise
	//                                   return modified string
	return sub(1,i-1) + sub(i+len(st),p->l);
      }
    }
  return *this;
}


SFString SFString::rem(const ulong i, const ulong n) {
  // remove n char starting at location [i-1]
  if (p && n > 0) {
    /* if start is past end -- do nothing */
    if (i > p->l) {
      return *this;
    } else if (i < 1) {
      /* if start is negative -- do nothing */
      return *this;
    } else if (i == 1 && n >= p->l) {
      /* remove all of string */
      return "";
    }
    /* start of second part of string */
    ulong i2 = min(i+n, p->l);
    if (i == 1) {
      /* first part of string is cut off */
      return sub(i2, p->l);
    } else if (i2 == p->l) {
      /* second part of string is cut off */
      return sub(1, i-1);
    } else {
      /* middle of string is cut out */
      return sub(1, i-1) + sub(i2+1, p->l);
    }
  }
  return *this;
}

SFString SFString::ins(const SFString& st, const unsigned int l) {
  // inserts argument string into this string at position l and
  // returns result

  if ( p )
    {
      if (l > 1) {
	/* insert in the middle of string */
	return sub(1,l-1) + st + sub(l,p->l) ;
      } else if (l) {
	/* insert at beginning of string */
	return st + sub(1, p->l);
      } else {
	/* do nothing */
	return *this;
      }
    }
  return *this;
}


SFString SFString::rep(const SFString& s1, const SFString& s2) {
  // replaces first occurence of s1 with s2 in this string and 
  // returns result
  int i = Index(s1);
  return rem(s1).ins(s2,i);
}

SFString SFString::subs(const SFString& s1, const SFString& s2) {
  // replaces all occurences of s1 with s2 in this string and 
  // returns result
  int i;
  SFString temp(*this);

  while((i = temp.Index(s1)) != 0)
    temp = temp.rem(s1).ins(s2,i);

  return temp;
}

SFString SFString::strip(char c) {

  char cc[2];

  *cc = c;
  *(cc+1) = '\0';

  SFString cs(cc);

  switch (c) {
  case '\'':
  case '"':
  default:
    if (p && sub(1,1) == cs && sub(p->l,p->l) == cs)
      return sub(2,p->l-1);
    else
      return *this;
  case '(':
  case ')':
    if (p && sub(1,1) == "(" && sub(p->l,p->l) == ")")
      return sub(2,p->l-1);
    else
      return *this;
  }
}

SFString SFString::quote() {
  char qt[1];

  *qt = '"';
  *(qt+1) = '\0';

  SFString sqt = SFString(qt);
  
  return sqt + *this + sqt;
}

SFString SFString::crop() {
  /* removes leading and trailing spaces and returns the result */
  
  int nTrailSpace = 0;

  if ( p )
    {
      while (p->s[p->l - nTrailSpace - 1] == ' ')
	nTrailSpace++;
      
      int nLeadSpace = strspn(p->s, " ");
      
      return SFString(*this, nLeadSpace+1, p->l - nTrailSpace);
    }
  return *this;
}


double SFString::Double() const {
  /* return the double conversion */

  if ( p )
    {
      return strtod(p->s, 0);
    }
  return 0.0;
}

slong SFString::Long() const {
  /* return long integer conversion */

  if ( p )
    {
      return slong(atoi(p->s));
    }
  return slong(0);
}

short SFString::Short() const {
  /* return short integer conversion */

  if ( p )
    {
      return short(atoi(p->s));
    }
  return 0;
}

int SFString::Int() const {
  /* return integer conversion */

  if ( p )
    {
      return atoi(p->s);
    }
  return 0;
}

int SFString::match(const SFString& pat) {
  // returns boolean value if this matches pattern, which may include
  // "*" characters representing wildcards.
  // string should not include "$"
  SFString t, p;
  SFString lf,rt;
  unsigned int not_end = 1;
  unsigned int mtch = 0;
  unsigned int i, j, k, l, inext, ls;

  t = SFString("$") + *this + "$";       // enclose strings in "$" delimiters
  p = SFString("$") + pat + "$";         //
  
  i = p.Index("*");                    // store position of first "*"

  if (i==0) { i = len(p); }            // if no "*" set i to last character

  lf = p.sub(1,i-1);                   // store string to left of i

  /* if left string of pattern does not match same number in this return 0 */
  if (lf != t.sub(1,i-1)) { return 0; }

  /*   otherwise iterate the procedure  */
  while (not_end) {

    /* start j at the first position after the '*' */
    j = i + 1;

    /* begin searching for the next '*' or end */
    k = i + p.sub(i+1,len(p)).Index("*");

    /* if no additional '*' put k at end */
    if (k == i) { k = len(p) + 1; }

    /* inext marks the next occurrence of '*' or end */
    inext = k;

    /* store the string between the '*'s or end for search */
    rt = p.sub(j,k-1);

    /* store the length of the search string */
    l = len(rt);

    /* store the last possible position of the search string in t */
    ls = len(t) - l + 1;

    if (len(t) < l)
      return 0;

    /* store position of next segment of pattern */
    k = j + t.sub(j,len(t)).Index(rt.p->s) - 1;
    
    /* if search string is not found then point to last possible position */
    if (k == j-1) { k = ls + 1; }
		    
    if (k == ls) {
      // if pointer is at last possible position, then search string
      // should fill out remainder of string -- we are done
      p = p.sub(1,i-1) + t.sub(i,len(t));
      not_end = 0;
    } else {
      // if k is beyond ls then we could not match the search string
      // and we are done -- test out of loop
      // otherwise, continue
      not_end = (k <= ls);

      /* check for a match of the search string in case we fell through */
      mtch = (rt.sub(1,l) == t.sub(k,k+l-1));

      /* if a match is made substitute it in and move the pointer */
      if (mtch) {
	p = p.rep("*",t.sub(i,k-1).p->s);
	i = inext + k - i - 1;
	lf = rt;
      }
    }
  }
  return (p == t);
}

int SFString::NTokens(const SFString& del) const {
  /* counts the number of distinct tokens in this string as delimited by del */

  if ( !p || !del.p )
    {
      return 0;
    }

  char* t = new char[p->l+1];
  strcpy(t,p->s);
  char* s = strtok(t, del.p->s);
  
  int i = 1;
  while ((s = strtok((char*) 0, del.p->s)) != 0)
    i++;
  delete [] t;
  
  return i;
}

int SFString::TokenList(SFString*& tokens, const SFString& del, slong** loc) const {
  // separates this string into a an array of strings where the strings are 
  // assumed to be separated by the delimiter character 

  if ( !p || !del.p )
    {
      return 0;
    }

  int n = NTokens(del);

  tokens = new SFString[n];

  if ( tokens )
    {
      char* tokstr = strtok(p->s, del.p->s);
      *tokens = tokstr;
      
      if (loc) {
	*loc = new slong[n];
	(*loc)[0] = tokstr - p->s;
      }
      
      for (int j = 1; j < n; j++) {
	tokstr = strtok((char*) 0, del.p->s);
	tokens[j] = tokstr;
	if (loc)
	  (*loc)[j] = tokstr - p->s;
      }
    }
  return n;
}

int SFString::InTokenList(SFString list, const SFString del) const {
  // returns true if this String is contained as a whole word in the [del]
  // delimited list.

  if ( !p )
    {
      return 0;
    }

  return *this == list || list.Index(*this + del) == 1 || 
    (list.Index(del + *this) == list.p->l - del.p->l - p->l + 1 && 
     list.Index(del + *this) > 0) || list.Index(del + *this + del) > 0;
}

SFString SFString::TokenString(const SFString* sourceList,
			   const int nwords, const SFString del) {
  // given an array of strings, create a single string concatenation using
  // [del] as a delimiter string

  if (!nwords)
    return SFString("");

  *this = sourceList[0];

  int i;
  for (i=1; i<nwords; i++)
    *this = *this + del + sourceList[i];

  return *this;
}

SFString SFString::StHexToStInt() {
  /* convert from string hex to string integer and return value */

  if ( !p )
    {
      return 0;
    }  

  char* c = p->s;
  slong n = 0;
  while (isxdigit(*c)) {
    n *= 16;
    n += isdigit(*c)? *c - '0': 10 + ((isupper(*c))? *c - 'A': *c - 'a');
    c++;
  }
  c = new char[12];
  if ( c )
    {
      sprintf(c, "%ld", n);
    } 
  return c;
}

slong SFString::StHexToInt() {
  /* convert from string hex to long and return value */

  if ( !p )
    {
      return 0;
    }

  char* c = p->s;
  slong n = 0;
  while (isxdigit(*c)) {
    n *= 16;
    n += isdigit(*c)? *c - '0': 10 + ((isupper(*c))? *c - 'A': *c - 'a');
    c++;
  }

  return n;
}


void Catalog::TabulateHeader(DataDocument* doc, SFString title,
			     SFString* pattern) {
  short i;
  
  if (title.IsNull())
    title = SFString("name");

  if ( doc )
    {
      doc->OpenRow();
      
      doc->WriteCell("name");
      
      for (i=0; i<n; i++) {
	
	if (!pattern ||
	    pattern && name[i].match(*pattern)) {
	  doc->WriteCell(name[i]);
	}
      }
      
      doc->CloseRow();
    }
}

void Catalog::Tabulate(DataDocument* doc, SFString title, SFString* pattern) {
  short i;

  if ( doc )
    {   
      doc->OpenRow();
      doc->WriteCell(title);
      for (i=0; i<n; i++) {
	
	if (!pattern ||
	    pattern && name[i].match(*pattern)) {
	  
	  switch(etype[i]) {
	  case string_element:
	    doc->WriteCell(value[i].str);
	    break;
	  case numeric_element:
	    doc->WriteCell(value[i].number);
	    break;
	  case list_element:
	    doc->WriteCell("<list>");
	    break;
	  }
	}
      }
      doc->CloseRow();
    }
}
