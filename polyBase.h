/*
  @copyright Steve Keen 2012
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/
///object base support for polymophic types used with shared_ptrs

#ifndef POLYBASE_H
#define POLYBASE_H
#include <pack_base.h>
#include <xml_pack_base.h>
#include <xml_unpack_base.h>

namespace classdesc
{
  // a non template marker class for use in metaprogramming
  struct PolyBaseMarker {};

  /// T is the immediate base class of the polymorphic type
  /// Type is the enumeration type of the type identifier
  /// for some reason we cannot infer enums from passed classes
  template <class E>
  struct PolyBase: public PolyBaseMarker
  {
    virtual E type() const =0;
    virtual PolyBase* clone() const=0;
    virtual void pack(pack_t&)=0;
    virtual void unpack(unpack_t&)=0;
    virtual void xml_pack(xml_pack_t&, const string&)=0;
    virtual void xml_unpack(xml_unpack_t&, const string&)=0;
    virtual void TCL_obj(TCL_obj_t& t, const string& d)=0;
    virtual ~PolyBase() {}
  };


  /// T is derived class, B its immediate polymorphic base
  /// B must derive from PolyBase
  template <class T, class B>
  struct PolyBaseT: public B
  {
    PolyBaseT* clone() const {return new T(static_cast<const T&>(*this));}
    void pack(pack_t& x) 
    {::pack(x,"",static_cast<T&>(*this));}
    void unpack(unpack_t& x) 
    {::unpack(x,"",static_cast<T&>(*this));}
    void xml_pack(xml_pack_t& x, const string& d) 
    {::xml_pack(x,d,static_cast<T&>(*this));}
    void xml_unpack(xml_unpack_t& x, const string& d)
    {::xml_unpack(x,d,static_cast<T&>(*this));}
    void TCL_obj(TCL_obj_t& t, const string& d)
    {::TCL_obj(t,d,static_cast<T&>(*this));}
  };

  namespace descriptors
  {
    template <class T>
    struct isPoly
    {
      static const bool value=
        classdesc::is_base_of<classdesc::PolyBaseMarker, T>::value;
    };

    template <class T>
    typename classdesc::enable_if<isPoly<T>, void>::T
    pack(pack_t& x, shared_ptr<T>& a)
    {
      if (classdesc::PolyBase<typename T::Type>* p=
          dynamic_cast<classdesc::PolyBase<typename T::Type>*>(a.get()))
        {
          ::pack(x,"",p->type());
          p->pack(x);
        }
      else
        ::pack(x,"",false);
    }

    template <class T>
    typename classdesc::enable_if<Not<isPoly<T> >, void>::T
    pack(pack_t& x, shared_ptr<T>& a)
    {
      ::pack(x,"",(bool)a);
      if (a) ::pack(x,"",*a);
    }

    template <class T>
    typename enable_if<isPoly<T>, void>::T
    unpack(unpack_t& x, shared_ptr<T>& a)
    {
      bool valid;
      ::unpack(x,"",valid);
      if (valid)
        {
          typename T::Type type;
          ::unpack(x,"",type);
          T* p=T::create(type);
          p->unpack(x);
          a.reset(p);
        }
    }

    template <class T>
    typename enable_if<Not<isPoly<T> >, void>::T
    unpack(unpack_t& x, shared_ptr<T>& a)
    {
      bool valid;
      ::unpack(x,"",valid);
      if (valid)
        {
          a.reset(new T);
          ::unpack(x,"",*a);
        }
      else
        a.reset();
    }

    template <class T>
    typename enable_if<isPoly<T>, void>::T
    xml_pack(xml_pack_t& x, const string& d, shared_ptr<T>& a)
    {
      if (classdesc::PolyBase<typename T::Type>* p=
          dynamic_cast<classdesc::PolyBase<typename T::Type>*>(a.get()))
        {
          ::xml_pack(x,d+".m_type",p->type());
          p->xml_pack(x,d);
        }
    }

    template <class T>
    typename classdesc::enable_if<Not<isPoly<T> >, void>::T
    xml_pack(xml_pack_t& x, const string& d, shared_ptr<T>& a)
    {
      if (a) ::xml_pack(x,d,a);
    }

    template <class T>
    typename enable_if<isPoly<T>, void>::T
    xml_unpack(xml_unpack_t& x, const string& d, shared_ptr<T>& a)
    {
      if (x.count(d))
        {
          typename T::Type type;
          //TODO: remove leading m_
          ::xml_unpack(x,d+".m_type",type);
          T* p=T::create(type);
          a.reset(p);
          ::xml_unpack(x,d,*p);
        }
      else
        {
          std::cout << d << std::endl;
          // no data, reset pointer
          a.reset();
        }
    }

    template <class T>
    typename enable_if<Not<isPoly<T> >, void>::T
    xml_unpack(xml_unpack_t& x, const string& d, shared_ptr<T>& a)
    {
      if (x.count(d))
        {
          a.reset(new T);
          ::xml_unpack(x,d,*a);
        }
      else
        {
          // no data, reset pointer.
          a.reset();
        }
    }      
  
    
    template <class T>
    typename enable_if<isPoly<T>, void>::T
    TCL_obj(TCL_obj_t& x, const string& d, shared_ptr<T>& a)
    {if (a)  a->TCL_obj(x,d);}

    template <class T>
    typename enable_if<Not<isPoly<T> >, void>::T
    TCL_obj(TCL_obj_t& x, const string& d, shared_ptr<T>& a)
    {if (a) ::TCL_obj(x,d,*a);}
  }
}

namespace classdesc_access
{
  template <class T>
  struct access_pack<classdesc::shared_ptr<T> >
  {
    void operator()(classdesc::pack_t& x, const classdesc::string& d, 
               classdesc::shared_ptr<T>& a)
    {classdesc::descriptors::pack(x,a);}    
  };

  template <class T>
  struct access_unpack<classdesc::shared_ptr<T> >
  {
    void operator()(classdesc::unpack_t& x, const classdesc::string& d, 
               classdesc::shared_ptr<T>& a)
    {classdesc::descriptors::unpack(x,a);}
  };

  template <class T>
  struct access_xml_pack<classdesc::shared_ptr<T> >
  {
    void operator()(classdesc::xml_pack_t& x, const classdesc::string& d, 
               classdesc::shared_ptr<T>& a)
    {classdesc::descriptors::xml_pack(x,d,a);}
  };

  template <class T>
  struct access_xml_unpack<classdesc::shared_ptr<T> >
  {
    void operator()(classdesc::xml_unpack_t& x, const classdesc::string& d, 
               classdesc::shared_ptr<T>& a)
    {
      classdesc::descriptors::xml_unpack(x,d,a);
      assert(a);
    }
  };

  template <class T>
  struct access_TCL_obj<classdesc::shared_ptr<T> >
  {
    void operator()(classdesc::TCL_obj_t& x, const classdesc::string& d, 
               classdesc::shared_ptr<T>& a)
    {
      classdesc::descriptors::TCL_obj(x,d,a);
    }
  };

}

#include "polyBase.cd"
#endif
