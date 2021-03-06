/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __TRACKEDELEMENT_H__
#define __TRACKEDELEMENT_H__

#include "config.h"

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <stdexcept>

#include <vector>
#include <map>

#include "macaddr.h"
#include "uuid.h"

// Type safety can be disabled by commenting out this definition.  This will no
// longer validate that the type of element matches the use; if used improperly this
// will lead to "interesting" errors (for simple types) or segfaults (with complex
// types, either due to a null pointer or due to overlapping union values in the
// complex pointer).
//
// On the flip side, validating the type is one of the most commonly called 
// functions, and if this presents a problem, turning off type checking can cull 
// a large percentage of the function calls

#define TE_TYPE_SAFETY  1

#ifndef TE_TYPE_SAFETY
// If there's no type safety, define an empty except_type_mismatch
#define except_type_mismatch(V) ;
#endif

class GlobalRegistry;
class EntryTracker;

// Types of fields we can track and automatically resolve
// Statically assigned type numbers which MUST NOT CHANGE as things go forwards for 
// binary/fast serialization, new types must be added to the end of the list
enum TrackerType {
    TrackerUnassigned = -1,

    TrackerString = 0,

    TrackerInt8 = 1, 
    TrackerUInt8 = 2,

    TrackerInt16 = 3, 
    TrackerUInt16 = 4,

    TrackerInt32 = 5, 
    TrackerUInt32 = 6,

    TrackerInt64 = 7,
    TrackerUInt64 = 8,

    TrackerFloat = 9,
    TrackerDouble = 10,

    // Less basic types
    TrackerMac = 11, 
    TrackerUuid = 12,

    // Vector and named map
    TrackerVector = 13, 
    TrackerMap = 14,

    // unsigned integer map (int-keyed data not field-keyed)
    TrackerIntMap = 15,

    // Mac map (mac-keyed tracker data)
    TrackerMacMap = 16,

    // String-keyed map
    TrackerStringMap = 17,
    
    // Double-keyed map
    TrackerDoubleMap = 18,
};

class TrackerElement {
public:
    TrackerElement() {
        this->type = TrackerUnassigned;
        reference_count = 0;

        set_id(-1);

        // Redundant I guess
        dataunion.string_value = NULL;
        dataunion.int8_value = 0;
        dataunion.uint8_value = 0;
        dataunion.int16_value = 0;
        dataunion.uint16_value = 0;
        dataunion.int32_value = 0;
        dataunion.uint32_value = 0;
        dataunion.int64_value = 0;
        dataunion.uint64_value = 0;
        dataunion.float_value = 0.0f;
        dataunion.double_value = 0.0f;

        dataunion.mac_value = NULL;
        dataunion.uuid_value = NULL;

        dataunion.submap_value = NULL;
        dataunion.subintmap_value = NULL;
        dataunion.submacmap_value = NULL;
        dataunion.substringmap_value = NULL;
        dataunion.subdoublemap_value = NULL;
        dataunion.subvector_value = NULL;
        dataunion.custom_value = NULL;
    }

    TrackerElement(TrackerType type);
    TrackerElement(TrackerType type, int id);

    virtual ~TrackerElement();

    // Factory-style for easily making more of the same if we're subclassed
    virtual TrackerElement *clone_type() {
        return new TrackerElement(get_type(), get_id());
    }

    virtual TrackerElement *clone_type(int in_id) {
        TrackerElement *dupl = clone_type();
        dupl->set_id(in_id);

        return dupl;
    }

    // Called prior to serialization output
    virtual void pre_serialize() { }

    int get_id() {
        return tracked_id;
    }

    void set_id(int id) {
        tracked_id = id;
    }

    void set_local_name(string in_name) {
        local_name = in_name;
    }

    string get_local_name() {
        return local_name;
    }

    void link() {
        reference_count++;
    }

    void unlink() {
        reference_count--;

        // what?
        if (reference_count < 0) {
            throw std::runtime_error("tracker element link count < 0");
        }

        // Time to go
        if (reference_count == 0) {
            delete(this);
        }
    }

    int get_links() {
        return reference_count;
    }

    void set_type(TrackerType type);

    TrackerType get_type() { return type; }

    // Getter per type, use templated GetTrackerValue() for easy fetch
    string get_string() {
        except_type_mismatch(TrackerString);
        return *(dataunion.string_value);
    }

    uint8_t get_uint8() {
        except_type_mismatch(TrackerUInt8);
        return dataunion.uint8_value;
    }

    int8_t get_int8() {
        except_type_mismatch(TrackerInt8);
        return dataunion.int8_value;
    }

    uint16_t get_uint16() {
        except_type_mismatch(TrackerUInt16);
        return dataunion.uint16_value;
    }

    int16_t get_int16() {
        except_type_mismatch(TrackerInt16);
        return dataunion.int16_value;
    }

    uint32_t get_uint32() {
        except_type_mismatch(TrackerUInt32);
        return dataunion.uint32_value;
    }

    int32_t get_int32() {
        except_type_mismatch(TrackerInt32);
        return dataunion.int32_value;
    }

    uint64_t get_uint64() {
        except_type_mismatch(TrackerUInt64);
        return dataunion.uint64_value;
    }

    int64_t get_int64() {
        except_type_mismatch(TrackerInt64);
        return dataunion.int64_value;
    }

    float get_float() {
        except_type_mismatch(TrackerFloat);
        return dataunion.float_value;
    }

    double get_double() {
        except_type_mismatch(TrackerDouble);
        return dataunion.double_value;
    }

    mac_addr get_mac() {
        except_type_mismatch(TrackerMac);
        return (*(dataunion.mac_value));
    }

    vector<TrackerElement *> *get_vector() {
        except_type_mismatch(TrackerVector);
        return dataunion.subvector_value;
    }

    TrackerElement *get_vector_value(unsigned int offt) {
        except_type_mismatch(TrackerVector);
        return (*dataunion.subvector_value)[offt];
    }

    map<int, TrackerElement *> *get_map() {
        except_type_mismatch(TrackerMap);
        return dataunion.submap_value;
    }

    TrackerElement *get_map_value(int fn) {
        except_type_mismatch(TrackerMap);

        map<int, TrackerElement *>::iterator i = dataunion.submap_value->find(fn);

        if (i == dataunion.submap_value->end()) {
            return NULL;
        }

        return i->second;
    }

    map<int, TrackerElement *> *get_intmap() {
        except_type_mismatch(TrackerIntMap);
        return dataunion.subintmap_value;
    }

    map<mac_addr, TrackerElement *> *get_macmap() {
        except_type_mismatch(TrackerMacMap);
        return dataunion.submacmap_value;
    }

    map<string, TrackerElement *> *get_stringmap() {
        except_type_mismatch(TrackerStringMap);
        return dataunion.substringmap_value;
    }

    map<double, TrackerElement *> *get_doublemap() {
        except_type_mismatch(TrackerDoubleMap);
        return dataunion.subdoublemap_value;
    }

    uuid get_uuid() {
        except_type_mismatch(TrackerUuid);
        return *(dataunion.uuid_value);
    }

    // Overloaded set
    void set(string v) {
        except_type_mismatch(TrackerString);
        *(dataunion.string_value) = v;
    }

    void set(uint8_t v) {
        except_type_mismatch(TrackerUInt8);
        dataunion.uint8_value = v;
    }

    void set(int8_t v) {
        except_type_mismatch(TrackerInt8);
        dataunion.int8_value = v;
    }

    void set(uint16_t v) {
        except_type_mismatch(TrackerUInt16);
        dataunion.uint16_value = v;
    }

    void set(int16_t v) {
        except_type_mismatch(TrackerInt16);
        dataunion.int16_value = v;
    }

    void set(uint32_t v) {
        except_type_mismatch(TrackerUInt32);
        dataunion.uint32_value = v;
    }

    void set(int32_t v) {
        except_type_mismatch(TrackerInt32);
        dataunion.int32_value = v;
    }

    void set(uint64_t v) {
        except_type_mismatch(TrackerUInt64);
        dataunion.uint64_value = v;
    }

    void set(int64_t v) {
        except_type_mismatch(TrackerInt64);
        dataunion.int64_value = v;
    }

    void set(float v) {
        except_type_mismatch(TrackerFloat);
        dataunion.float_value = v;
    }

    void set(double v) {
        except_type_mismatch(TrackerDouble);
        dataunion.double_value = v;
    }

    void set(mac_addr v) {
        except_type_mismatch(TrackerMac);
        // mac has overrided =
        *(dataunion.mac_value) = v;
    }

    void set(uuid v) {
        except_type_mismatch(TrackerUuid);
        // uuid has overrided =
        *(dataunion.uuid_value) = v;
    }

    size_t size();

    typedef vector<TrackerElement *> tracked_vector;
    typedef vector<TrackerElement *>::iterator vector_iterator;
    typedef vector<TrackerElement *>::const_iterator vector_const_iterator;

    typedef map<int, TrackerElement *> tracked_map;
    typedef map<int, TrackerElement *>::iterator map_iterator;
    typedef map<int, TrackerElement *>::const_iterator map_const_iterator;
    typedef pair<int, TrackerElement *> tracked_pair;

    typedef map<int, TrackerElement *> tracked_int_map;
    typedef map<int, TrackerElement *>::iterator int_map_iterator;
    typedef map<int, TrackerElement *>::const_iterator int_map_const_iterator;
    typedef pair<int, TrackerElement *> int_map_pair;

    typedef map<mac_addr, TrackerElement *> tracked_mac_map;
    typedef map<mac_addr, TrackerElement *>::iterator mac_map_iterator;
    typedef map<mac_addr, TrackerElement *>::const_iterator mac_map_const_iterator;
    typedef pair<mac_addr, TrackerElement *> mac_map_pair;

    typedef map<string, TrackerElement *> tracked_string_map;
    typedef map<string, TrackerElement *>::iterator string_map_iterator;
    typedef map<string, TrackerElement *>::const_iterator string_map_const_iterator;
    typedef pair<string, TrackerElement *> string_map_pair;

    typedef map<double, TrackerElement *> tracked_double_map;
    typedef map<double, TrackerElement *>::iterator double_map_iterator;
    typedef map<double, TrackerElement *>::const_iterator double_map_const_iterator;
    typedef pair<double, TrackerElement *> double_map_pair;

    vector_iterator vec_begin();
    vector_iterator vec_end();

    map_iterator begin();
    map_iterator end();
    map_iterator find(int k);
    void clear_map();
    size_t size_map();

    void add_map(int f, TrackerElement *s);
    void add_map(TrackerElement *s); 
    void del_map(int f);
    void del_map(TrackerElement *s);
    void del_map(map_iterator i);
    void insert_map(tracked_pair p);

    void add_intmap(int i, TrackerElement *s);
    void del_intmap(int i);
    void del_intmap(int_map_iterator i);
    void clear_intmap();
    void insert_intmap(int_map_pair p);
    size_t size_intmap();

    TrackerElement *get_intmap_value(int idx);
    int_map_iterator int_begin();
    int_map_iterator int_end();
    int_map_iterator int_find(int k);

    void add_macmap(mac_addr i, TrackerElement *s);
    void del_macmap(mac_addr i);
    void del_macmap(mac_map_iterator i);
    void clear_macmap();
    void insert_macmap(mac_map_pair p);
    size_t size_macmap();

    TrackerElement *get_macmap_value(int idx);
    mac_map_iterator mac_begin();
    mac_map_iterator mac_end();
    mac_map_iterator mac_find(mac_addr k);

    void add_stringmap(string i, TrackerElement *s);
    void del_stringmap(string i);
    void del_stringmap(string_map_iterator i);
    void clear_stringmap();
    void insert_stringmap(string_map_pair p);
    size_t size_stringmap();

    TrackerElement *get_stringmap_value(string idx);
    string_map_iterator string_begin();
    string_map_iterator string_end();
    string_map_iterator string_find(string k);

    void add_doublemap(double i, TrackerElement *s);
    void del_doublemap(double i);
    void del_doublemap(double_map_iterator i);
    void clear_doublemap();
    void insert_doublemap(double_map_pair p);
    size_t size_doublemap();

    TrackerElement *get_doublemap_value(double idx);
    double_map_iterator double_begin();
    double_map_iterator double_end();
    double_map_iterator double_find(double k);

    void add_vector(TrackerElement *s);
    void del_vector(unsigned int p);
    void del_vector(vector_iterator i);
    void clear_vector();
    size_t size_vector();

    // Do our best to increment a value
    TrackerElement& operator++(const int);

    // Do our best to decrement a value
    TrackerElement& operator--(const int);

    // Do our best to do compound addition
    TrackerElement& operator+=(const int& v);
    TrackerElement& operator+=(const unsigned int& v);
    TrackerElement& operator+=(const float& v);
    TrackerElement& operator+=(const double& v);

    TrackerElement& operator+=(const int64_t& v);
    TrackerElement& operator+=(const uint64_t& v);

    // Do our best to do compound subtraction
    TrackerElement& operator-=(const int& v);
    TrackerElement& operator-=(const unsigned int& v);
    TrackerElement& operator-=(const float& v);
    TrackerElement& operator-=(const double& v);

    TrackerElement& operator-=(const int64_t& v);
    TrackerElement& operator-=(const uint64_t& v);

    // Do our best for equals comparison
    
    // Comparing tracked elements themselves presents weird problems - how do we deal with 
    // conflicting ids but equal data?  Lets see if we actually need it.  /D
    // friend bool operator==(TrackerElement &te1, TrackerElement &te2);

    friend bool operator==(TrackerElement &te1, int8_t i);
    friend bool operator==(TrackerElement &te1, uint8_t i);
    friend bool operator==(TrackerElement &te1, int16_t i);
    friend bool operator==(TrackerElement &te1, uint16_t i);
    friend bool operator==(TrackerElement &te1, int32_t i);
    friend bool operator==(TrackerElement &te1, uint32_t i);
    friend bool operator==(TrackerElement &te1, int64_t i);
    friend bool operator==(TrackerElement &te1, uint64_t i);
    friend bool operator==(TrackerElement &te1, float f);
    friend bool operator==(TrackerElement &te1, double d);
    friend bool operator==(TrackerElement &te1, mac_addr m);
    friend bool operator==(TrackerElement &te1, uuid u);

    friend bool operator<(TrackerElement &te1, int8_t i);
    friend bool operator<(TrackerElement &te1, uint8_t i);
    friend bool operator<(TrackerElement &te1, int16_t i);
    friend bool operator<(TrackerElement &te1, uint16_t i);
    friend bool operator<(TrackerElement &te1, int32_t i);
    friend bool operator<(TrackerElement &te1, uint32_t i);
    friend bool operator<(TrackerElement &te1, int64_t i);
    friend bool operator<(TrackerElement &te1, uint64_t i);
    friend bool operator<(TrackerElement &te1, float f);
    friend bool operator<(TrackerElement &te1, double d);
    friend bool operator<(TrackerElement &te1, mac_addr m);
    friend bool operator<(TrackerElement &te1, uuid u);

    friend bool operator>(TrackerElement &te1, int8_t i);
    friend bool operator>(TrackerElement &te1, uint8_t i);
    friend bool operator>(TrackerElement &te1, int16_t i);
    friend bool operator>(TrackerElement &te1, uint16_t i);
    friend bool operator>(TrackerElement &te1, int32_t i);
    friend bool operator>(TrackerElement &te1, uint32_t i);
    friend bool operator>(TrackerElement &te1, int64_t i);
    friend bool operator>(TrackerElement &te1, uint64_t i);
    friend bool operator>(TrackerElement &te1, float f);
    friend bool operator>(TrackerElement &te1, double d);
    // We don't have > operators on mac or uuid
   
    // Bitwise
    TrackerElement& operator|=(int8_t i);
    TrackerElement& operator|=(uint8_t i);
    TrackerElement& operator|=(int16_t i);
    TrackerElement& operator|=(uint16_t i);
    TrackerElement& operator|=(int32_t i);
    TrackerElement& operator|=(uint32_t i);
    TrackerElement& operator|=(int64_t i);
    TrackerElement& operator|=(uint64_t i);

    TrackerElement& operator&=(int8_t i);
    TrackerElement& operator&=(uint8_t i);
    TrackerElement& operator&=(int16_t i);
    TrackerElement& operator&=(uint16_t i);
    TrackerElement& operator&=(int32_t i);
    TrackerElement& operator&=(uint32_t i);
    TrackerElement& operator&=(int64_t i);
    TrackerElement& operator&=(uint64_t i);

    TrackerElement& operator^=(int8_t i);
    TrackerElement& operator^=(uint8_t i);
    TrackerElement& operator^=(int16_t i);
    TrackerElement& operator^=(uint16_t i);
    TrackerElement& operator^=(int32_t i);
    TrackerElement& operator^=(uint32_t i);
    TrackerElement& operator^=(int64_t i);
    TrackerElement& operator^=(uint64_t i);

    TrackerElement *operator[](int i);
    TrackerElement *operator[](mac_addr i);

    static string type_to_string(TrackerType t);

protected:
    // Generic coercion exception
#ifdef TE_TYPE_SAFETY
    inline void except_type_mismatch(const TrackerType t) const {
        if (type != t) {
            string w = "element type mismatch, is " + type_to_string(this->type) + 
                " tried to use as " + type_to_string(t);

            throw std::runtime_error(w);
        }
    }
#endif

    // Garbage collection?  Say it ain't so...
    int reference_count;

    TrackerType type;
    int tracked_id;

    // Overridden name for this instance only
    string local_name;

    // We could make these all one type, but then we'd have odd interactions
    // with incrementing and I'm not positive that's safe in all cases
    union {
        string *string_value;

        uint8_t uint8_value;
        int8_t int8_value;

        uint16_t uint16_value;
        int16_t int16_value;

        uint32_t uint32_value;
        int32_t int32_value;

        uint64_t uint64_value;
        int64_t int64_value;

        float float_value;
        double double_value;

        // Field ID,Element keyed map
        map<int, TrackerElement *> *submap_value;

        // Index int,Element keyed map
        map<int, TrackerElement *> *subintmap_value;

        // Index mac,element keyed map
        map<mac_addr, TrackerElement *> *submacmap_value;

        // Index string,element keyed map
        map<string, TrackerElement *> *substringmap_value;

        // Index double,element keyed map
        map<double, TrackerElement *> *subdoublemap_value;

        vector<TrackerElement *> *subvector_value;

        mac_addr *mac_value;

        uuid *uuid_value;

        void *custom_value;
    } dataunion;

};

// Helper child classes
class TrackerElementVector {
protected:
    TrackerElement *val;

public:
    typedef TrackerElement::vector_iterator iterator;
    typedef TrackerElement::vector_const_iterator const_iterator;

    TrackerElementVector(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementVector() {
        val->unlink();
    }

    virtual iterator begin() {
        return val->vec_begin();
    }

    virtual iterator end() {
        return val->vec_end();
    }

    virtual void clear() {
        return val->clear_vector();
    }

    virtual void push_back(TrackerElement *i) {
        return val->add_vector(i);
    }

    virtual void erase(unsigned int p) {
        return val->del_vector(p);
    }

    virtual void erase(iterator i) {
        return val->del_vector(i);
    }

    virtual size_t size() {
        return val->size_vector();
    }

};

class TrackerElementMap {
protected:
    TrackerElement *val;

public:
    TrackerElementMap(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementMap() {
        val->unlink();
    }

public:
    typedef TrackerElement::map_iterator iterator;
    typedef TrackerElement::map_const_iterator const_iterator;
    typedef TrackerElement::tracked_pair pair;

    virtual iterator begin() {
        return val->begin();
    }

    virtual iterator end() {
        return val->end();
    }

    virtual iterator find(int k) {
        return val->find(k);
    }

    virtual void insert(pair p) {
        return val->insert_map(p);
    }

    virtual void erase(iterator i) {
        return val->del_map(i);
    }

    virtual void clear() {
        return val->clear_map();
    }

    virtual size_t size() {
        return val->size_map();
    }
};

class TrackerElementIntMap {
protected:
    TrackerElement *val;

public:
    TrackerElementIntMap(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementIntMap() {
        val->unlink();
    }

public:
    typedef TrackerElement::int_map_iterator iterator;
    typedef TrackerElement::int_map_const_iterator const_iterator;
    typedef TrackerElement::int_map_pair pair;

    virtual iterator begin() {
        return val->int_begin();
    }

    virtual iterator end() {
        return val->int_end();
    }

    virtual iterator find(int k) {
        return val->int_find(k);
    }

    virtual void insert(pair p) {
        return val->insert_intmap(p);
    }

    virtual void erase(iterator i) {
        return val->del_intmap(i);
    }

    virtual void clear() {
        return val->clear_intmap();
    }

    virtual size_t size() {
        return val->size_intmap();
    }
};

class TrackerElementStringMap {
protected:
    TrackerElement *val;

public:
    TrackerElementStringMap(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementStringMap() {
        val->unlink();
    }

public:
    typedef TrackerElement::string_map_iterator iterator;
    typedef TrackerElement::string_map_const_iterator const_iterator;
    typedef TrackerElement::string_map_pair pair;

    virtual iterator begin() {
        return val->string_begin();
    }

    virtual iterator end() {
        return val->string_end();
    }

    virtual iterator find(string k) {
        return val->string_find(k);
    }

    virtual void insert(pair p) {
        return val->insert_stringmap(p);
    }

    virtual void erase(iterator i) {
        return val->del_stringmap(i);
    }

    virtual void clear() {
        return val->clear_stringmap();
    }

    virtual size_t size() {
        return val->size_stringmap();
    }
};

class TrackerElementMacMap {
protected:
    TrackerElement *val;

public:
    TrackerElementMacMap(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementMacMap() {
        val->unlink();
    }

public:
    typedef TrackerElement::mac_map_iterator iterator;
    typedef TrackerElement::mac_map_const_iterator const_iterator;
    typedef TrackerElement::mac_map_pair pair;

    virtual iterator begin() {
        return val->mac_begin();
    }

    virtual iterator end() {
        return val->mac_end();
    }

    virtual iterator find(mac_addr k) {
        return val->mac_find(k);
    }

    virtual void insert(pair p) {
        return val->insert_macmap(p);
    }

    virtual void erase(iterator i) {
        return val->del_macmap(i);
    }

    virtual void clear() {
        return val->clear_macmap();
    }

    virtual size_t size() {
        return val->size_macmap();
    }
};

class TrackerElementDoubleMap {
protected:
    TrackerElement *val;

public:
    TrackerElementDoubleMap(TrackerElement *t) {
        val = t;
        val->link();
    }

    virtual ~TrackerElementDoubleMap() {
        val->unlink();
    }

public:
    typedef TrackerElement::double_map_iterator iterator;
    typedef TrackerElement::double_map_const_iterator const_iterator;
    typedef TrackerElement::double_map_pair pair;

    virtual iterator begin() {
        return val->double_begin();
    }

    virtual iterator end() {
        return val->double_end();
    }

    virtual iterator find(double k) {
        return val->double_find(k);
    }

    virtual void insert(pair p) {
        return val->insert_doublemap(p);
    }

    virtual void erase(iterator i) {
        return val->del_doublemap(i);
    }

    virtual void clear() {
        return val->clear_doublemap();
    }

    virtual size_t size() {
        return val->size_doublemap();
    }
};

// Templated access functions

template<typename T> T GetTrackerValue(TrackerElement *);

template<> string GetTrackerValue(TrackerElement *e);
template<> int8_t GetTrackerValue(TrackerElement *e);
template<> uint8_t GetTrackerValue(TrackerElement *e);
template<> int16_t GetTrackerValue(TrackerElement *e);
template<> uint16_t GetTrackerValue(TrackerElement *e);
template<> int32_t GetTrackerValue(TrackerElement *e);
template<> uint32_t GetTrackerValue(TrackerElement *e);
template<> int64_t GetTrackerValue(TrackerElement *e);
template<> uint64_t GetTrackerValue(TrackerElement *e);
template<> float GetTrackerValue(TrackerElement *e);
template<> double GetTrackerValue(TrackerElement *e);
template<> mac_addr GetTrackerValue(TrackerElement *e);
template<> map<int, TrackerElement *> *GetTrackerValue(TrackerElement *e);
template<> vector<TrackerElement *> *GetTrackerValue(TrackerElement *e);

// Complex trackable unit based on trackertype dataunion.
//
// All tracker_components are built from maps.
//
// Tracker components are stored via integer references, but the names are
// mapped via the entrytracker system.
//
// Sub-classes must initialize sub-fields by calling register_fields() in their
// constructors.  The register_fields() function is responsible for defining the
// types and builders, and recording the field_ids for all sub-fields and nested 
// components.
//
// Fields are allocated via the reserve_fields function, which must be called before
// use of the component.  By passing an existing trackermap object, a parsed tree
// can be annealed into the c++ representation without copying/re-parsing the data.
class tracker_component : public TrackerElement {

// Ugly trackercomponent macro for proxying trackerelement values
// Defines get_<name> function, for a TrackerElement of type <ptype>, returning type 
// <rtype>, referencing class variable <cvar>
// Defines set_<name> funciton, for a TrackerElement of type <ptype>, taking type 
// <itype>, which must be castable to the TrackerElement type (itype), referencing 
// class variable <cvar>
#define __Proxy(name, ptype, itype, rtype, cvar) \
    virtual TrackerElement *get_tracker_##name() { \
        return (TrackerElement *) cvar; \
    } \
    virtual rtype get_##name() const { \
        return (rtype) GetTrackerValue<ptype>(cvar); \
    } \
    virtual void set_##name(itype in) { \
        cvar->set((ptype) in); \
    }

// Only proxy a Get function
#define __ProxyGet(name, ptype, rtype, cvar) \
    virtual rtype get_##name() { \
        return (rtype) GetTrackerValue<ptype>(cvar); \
    } 

// Only proxy a Set function for overload
#define __ProxySet(name, ptype, stype, cvar) \
    virtual void set_##name(stype in) { \
        cvar->set((stype) in); \
    } 

// Proxy increment and decrement functions
#define __ProxyIncDec(name, ptype, rtype, cvar) \
    virtual void inc_##name() { \
        (*cvar)++; \
    } \
    virtual void inc_##name(rtype i) { \
        (*cvar) += (ptype) i; \
    } \
    virtual void dec_##name() { \
        (*cvar)--; \
    } \
    virtual void dec_##name(rtype i) { \
        (*cvar) -= (ptype) i; \
    }

// Proxy add/subtract
#define __ProxyAddSub(name, ptype, itype, cvar) \
    virtual void add_##name(itype i) { \
        (*cvar) += (ptype) i; \
    } \
    virtual void sub_##name(itype i) { \
        (*cvar) -= (ptype) i; \
    }

// Proxy sub-trackable (name, trackable type, class variable)
#define __ProxyTrackable(name, ttype, cvar) \
    virtual ttype *get_##name() { return cvar; } \
    virtual void set_##name(ttype *in) { \
        if (cvar != NULL) \
            cvar->unlink(); \
        cvar = in; \
        cvar->link(); \
    }  \
    virtual TrackerElement *get_tracker_##name() { \
        return (TrackerElement *) cvar; \
    } 

// Proxy bitset functions (name, trackable type, data type, class var)
#define __ProxyBitset(name, dtype, cvar) \
    virtual void bitset_##name(dtype bs) { \
        (*cvar) |= bs; \
    } \
    virtual void bitclear_##name(dtype bs) { \
        (*cvar) &= ~(bs); \
    } \
    virtual dtype bitcheck_##name(dtype bs) { \
        return (dtype) (GetTrackerValue<dtype>(cvar) & bs); \
    }

public:
    // Build a basic component.  All basic components are maps.
    // Set the field id automatically.
    tracker_component(GlobalRegistry *in_globalreg, int in_id);

    // Build a component with existing map
    tracker_component(GlobalRegistry *in_globalreg, int in_id, 
            TrackerElement *e __attribute__((unused)));

	virtual ~tracker_component();

    virtual void mutex_lock() {
        pthread_mutex_lock(&pthread_lock);
    }

    virtual void mutex_unlock() {
        pthread_mutex_unlock(&pthread_lock);
    }

    // Clones the type and preserves that we're a tracker component.  
    // Complex subclasses will replace this to function as builders of
    // their own complex types.
    virtual TrackerElement *clone_type();

    // Return the name via the entrytracker
    virtual string get_name();

    // Proxy getting any name via entry tracker
    virtual string get_name(int in_id);

    TrackerElement *get_child_path(string in_path);
    TrackerElement *get_child_path(std::vector<string> in_path);

protected:
    // Reserve a field via the entrytracker, using standard entrytracker build methods.
    // This field will be automatically assigned or created during the reservefields 
    // stage.
    int RegisterField(string in_name, TrackerType in_type, string in_desc, 
            void **in_dest);

    // Reserve a field via the entrytracker, using standard entrytracker build methods,
    // but do not assign or create during the reservefields stage.
    // This can be used for registering sub-components of maps which are not directly
    // instantiated as top-level fields.
    int RegisterField(string in_name, TrackerType in_type, string in_desc);

    // Reserve a field via the entrytracker, using standard entrytracker build methods.
    // This field will be automatically assigned or created during the reservefields 
    // stage.
    // You will nearly always want to use registercomplex below since fields with 
    // specific builders typically want to inherit from a subtype
    int RegisterField(string in_name, TrackerElement *in_builder, string in_desc, 
            void **in_dest);

    // Reserve a complex via the entrytracker, using standard entrytracker build methods.
    // This field will NOT be automatically assigned or built during the reservefields 
    // stage, callers should manually create these fields, importing from the parent
    int RegisterComplexField(string in_name, TrackerElement *in_builder, 
            string in_desc);

    // Register field types and get a field ID.  Called during record creation, prior to 
    // assigning an existing trackerelement tree or creating a new one
    virtual void register_fields() { }

    // Populate fields - either new (e == NULL) or from an existing structure which
    //  may contain a generic version of our data.
    // When populating from an existing structure, bind each field to this instance so
    //  that we can track usage and delete() appropriately.
    // Populate automatically based on the fields we have reserved, subclasses can 
    // override if they really need to do something special
    virtual void reserve_fields(TrackerElement *e);

    // Inherit from an existing element or assign a new one.
    // Add imported or new field to our map for use tracking.
    virtual TrackerElement *import_or_new(TrackerElement *e, int i);

    class registered_field {
        public:
            registered_field(int id, void **assign) { 
                this->id = id; 
                this->assign = (TrackerElement **) assign;
            }

            int id;
            TrackerElement** assign;
    };

    GlobalRegistry *globalreg;
    EntryTracker *tracker;

    vector<registered_field *> registered_fields;

    pthread_mutex_t pthread_lock;
};

class tracker_component_locker {
public:
    tracker_component_locker(tracker_component *c) {
        component = c;
        component->mutex_lock();
    }

    ~tracker_component_locker() {
        component->mutex_unlock();
    }

protected:
    tracker_component *component;
};

// Generic serializer class to allow easy swapping of serializers
class TrackerElementSerializer {
public:
    TrackerElementSerializer(GlobalRegistry *in_globalreg,
            std::stringstream &in_stream) : stream(in_stream) {
        globalreg = in_globalreg;
    }

    virtual ~TrackerElementSerializer() { }

    virtual void serialize(TrackerElement *in_elem) = 0;

    virtual void serialize(tracker_component *in_component) {
        serialize((TrackerElement *) in_component);
    }

protected:
    GlobalRegistry *globalreg;
    std::stringstream &stream;
};
        

#endif
