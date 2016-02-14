// Copyright satoren
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>
#include <map>
#include <cassert>
#include "kaguya/config.hpp"
#include "kaguya/lua_ref.hpp"

#include "kaguya/detail/lua_ref_impl.hpp"
#include "kaguya/detail/lua_table_def.hpp"

namespace kaguya
{
	class State;

	//! Reference to Lua userdata
	class  LuaUserData :public Ref::RegistoryRef,public LuaTableOrUserDataImpl<LuaUserData>, public LuaBasicTypeFunctions<LuaUserData>
	{
	public:
		operator LuaRef() {
			push(state());
			return LuaRef(state(), StackTop());
		}
		LuaUserData(lua_State* state, StackTop) :Ref::RegistoryRef(state, StackTop())
		{
		}
		LuaUserData(lua_State* state, const NewTable& table) :Ref::RegistoryRef(state, table)
		{
		}
		LuaUserData(lua_State* state) :Ref::RegistoryRef(state, NewTable())
		{
		}
		LuaUserData()
		{
		}

		void typecheck()
		{
			if (type() != TYPE_USERDATA)
			{
				except::typeMismatchError(state(), "not user data");
				Ref::RegistoryRef::unref();
			}
		}

		using Ref::RegistoryRef::isNilref;
		using Ref::RegistoryRef::operator==;
		using Ref::RegistoryRef::operator!=;
		using Ref::RegistoryRef::operator<=;
		using Ref::RegistoryRef::operator>=;
		using Ref::RegistoryRef::operator<;
		using Ref::RegistoryRef::operator>;

		using LuaTableOrUserDataImpl<LuaUserData>::operator[];
	};


	template<>	struct lua_type_traits<LuaUserData> {
		typedef LuaUserData get_type;
		typedef LuaUserData push_type;

		static bool strictCheckType(lua_State* l, int index)
		{
			return lua_type(l, index) == LUA_TUSERDATA;
		}
		static bool checkType(lua_State* l, int index)
		{
			return lua_type(l, index) == LUA_TUSERDATA || lua_isnil(l, index);
		}
		static LuaUserData get(lua_State* l, int index)
		{
			lua_pushvalue(l, index);
			return LuaUserData(l, StackTop());
		}
		static int push(lua_State* l, const LuaUserData& ref)
		{
			ref.push(l);
			return 1;
		}
	};
	template<>	struct lua_type_traits<const LuaUserData&> :lua_type_traits<LuaUserData> {};


	class LuaTable :public Ref::RegistoryRef ,public LuaTableImpl<LuaTable>, public LuaTableOrUserDataImpl<LuaTable>, public LuaBasicTypeFunctions<LuaTable>
	{
	public:
		operator LuaRef() {
			push(state());
			return LuaRef(state(), StackTop());
		}
		LuaTable(lua_State* state, StackTop) :Ref::RegistoryRef(state, StackTop())
		{
			typecheck();
		}
		LuaTable(lua_State* state, const NewTable& table) :Ref::RegistoryRef(state, table)
		{
			typecheck();
		}
		LuaTable(lua_State* state) :Ref::RegistoryRef(state, NewTable())
		{
			typecheck();
		}
		LuaTable()
		{
			typecheck();
		}

		void typecheck()
		{
			if (type() != TYPE_TABLE)
			{
				except::typeMismatchError(state(), "not table");
				Ref::RegistoryRef::unref();
			}
		}

		using Ref::RegistoryRef::isNilref;
		using Ref::RegistoryRef::operator==;
		using Ref::RegistoryRef::operator!=;
		using Ref::RegistoryRef::operator<=;
		using Ref::RegistoryRef::operator>=;
		using Ref::RegistoryRef::operator<;
		using Ref::RegistoryRef::operator>;

		using LuaTableImpl<LuaTable>::operator[];
		using LuaTableOrUserDataImpl<LuaTable>::operator[];
	};

	template<>	struct lua_type_traits<LuaTable> {
		typedef LuaTable get_type;
		typedef LuaTable push_type;

		static bool strictCheckType(lua_State* l, int index)
		{
			return lua_istable(l, index);
		}
		static bool checkType(lua_State* l, int index)
		{
			return lua_istable(l, index) || lua_isnil(l, index);
		}
		static LuaTable get(lua_State* l, int index)
		{
			lua_pushvalue(l, index);
			return LuaTable(l, StackTop());
		}
		static int push(lua_State* l, const LuaTable& ref)
		{
			ref.push(l);
			return 1;
		}
	};
	template<>	struct lua_type_traits<const LuaTable&> :lua_type_traits<LuaTable> {};

	/**
	* This class is the type returned by members of non-const LuaRef(Table) when directly accessing its elements.
	*/
	class TableKeyReference: public LuaVariantImpl<TableKeyReference>
	{
	public:

		int pushStackIndex(lua_State* state)const
		{
			push(state);
			return lua_gettop(state);
		}
		lua_State* state()const { return state_; }

		friend class LuaRef;
		friend class State;
		template<typename T> friend class LuaTableImpl;
	
		template<typename T>
		operator T()const {
			util::ScopedSavedStack save(state_);
			push(state_);
			return lua_type_traits<T>::get(state_,-1);
		}
		
		//! this is not copy.same assign from referenced value.
		TableKeyReference& operator=(const TableKeyReference& src)
		{
			lua_pushvalue(state_, key_index_);
			src.push();
			lua_settable(state_, table_index_);
			return *this;
		}


		//! assign from T
		template<typename T>
		TableKeyReference& operator=(const T& src)
		{
			lua_pushvalue(state_, key_index_);
			lua_type_traits<T>::push(state_,src);
			lua_settable(state_, table_index_);

			return *this;
		}
#if KAGUYA_USE_CPP11
		template<typename T>
		TableKeyReference& operator=(T&& src)
		{
			lua_pushvalue(state_, key_index_);
			lua_type_traits<typename traits::remove_reference<T>::type>::push(state_, std::forward<T>(src));
			lua_settable(state_, table_index_);

			return *this;
		}
#endif

		bool isNilref()const {
			if (!state_)
			{
				return false;
			}
			util::ScopedSavedStack save(state_);
			push(state_);
			return lua_isnoneornil(state_,-1);
		}

		//! register class metatable to lua and set to table
		template<typename T, typename P>
		void setClass(const ClassMetatable<T, P>& reg)
		{
			set_class(reg);
		}

		//! set function 
		template<typename T>
		void setFunction(T f)
		{
			lua_pushvalue(state_, key_index_);
			lua_type_traits<FunctorType>::push(state_, FunctorType(f));
			lua_settable(state_, table_index_);
		}

		//deprecate
		LuaRef getValue()const
		{
			util::ScopedSavedStack save(state_);
			push(state_);
			return lua_type_traits<LuaRef>::get(state_,-1);
		}
		template<typename T>
		typename lua_type_traits<T>::get_type get()const
		{
			util::ScopedSavedStack save(state_);
			push(state_);
			return lua_type_traits<T>::get(state_, -1);
		}

		int push(lua_State* state)const
		{
			if (state_ != state)
			{
				throw std::runtime_error("can not now");//fixme
			}
			if (lua_type(state_, table_index_) != LUA_TTABLE)
			{
				lua_pushnil(state_);
				return 1;
			}
			lua_pushvalue(state_, key_index_);
			lua_gettable(state_, table_index_);
			return 1;
		}

		int push()const
		{
			return push(state_);
		}

		operator LuaRef() const {
			return get<LuaRef>();
		}
		operator LuaTable() const {
			return get<LuaTable>();
		}
		operator LuaUserData() const {
			return get<LuaUserData>();
		}
		operator LuaFunction() const {
			return get<LuaFunction>();
		}
		operator LuaThread() const {
			return get<LuaThread>();
		}

		/**
		* @brief table->*"function_name"() in c++ and table:function_name(); in lua is same
		* @param function_name function_name in table
		*/
		mem_fun_binder operator->*(const char* function_name)
		{
			return get<LuaRef>()->*(function_name);
		}


		int type()const
		{
			util::ScopedSavedStack save(state_);
			push();
			return lua_type(state_, -1);
		}
		
		~TableKeyReference()
		{
			if(state_)
			{
				lua_settop(state_, stack_top_);
			}
		}

		bool operator==(const TableKeyReference& rhs)
		{
			return get<LuaRef>() == rhs.get<LuaRef>();
		}
		bool operator!=(const TableKeyReference& rhs)
		{
			return !(*this == rhs);
		}
		bool operator<(const TableKeyReference& rhs)
		{
			return get<LuaRef>() < rhs.get<LuaRef>();
		}
		bool operator<=(const TableKeyReference& rhs)
		{
			return get<LuaRef>() <= rhs.get<LuaRef>();
		}
		bool operator>=(const TableKeyReference& rhs)
		{
			return !(*this <= rhs);
		}
		bool operator>(const TableKeyReference& rhs)
		{
			return !(*this < rhs);
		}

		template<typename T>
		bool operator==(const T& rhs)
		{
			return get<T>() == rhs;
		}
		template<typename T>
		bool operator!=(const T& rhs)
		{
			return !(*this == rhs);
		}
		template<typename T>
		bool operator<(const T& rhs)
		{
			return get<T>() < rhs;
		}
		template<typename T>
		bool operator<=(const T& rhs)
		{
			return get<T>() <= rhs;
		}
		template<typename T>
		bool operator>=(const T& rhs)
		{
			return rhs <= *this;
		}
		template<typename T>
		bool operator>(const T& rhs)
		{
			return rhs < *this;
		}

	private:
		///!constructs the reference. Accessible only to kaguya::LuaRef itself 
		TableKeyReference(const TableKeyReference& src) : state_(src.state_), stack_top_(src.stack_top_), table_index_(src.table_index_), key_index_(src.key_index_)
		{
			src.state_ = 0;
		}
		
		template<typename T, typename P>
		void set_class(const ClassMetatable<T, P>& reg)
		{
			LuaRef table(state_, NewTable());
			table.setMetatable(reg.registerClass(state_));
			*this = table;
		}

		///!constructs the reference. Accessible only to kaguya::LuaRef itself 
		TableKeyReference(lua_State* state,int table_index,int key_index,int revstacktop) : state_(state), stack_top_(revstacktop), table_index_(table_index), key_index_(key_index)
		{
			if (lua_type(state_, table_index_) != LUA_TTABLE)
			{
				except::typeMismatchError(state_, "this is not table");
			}
		}

		///!constructs the reference. Accessible only to kaguya::LuaRef itself 
		TableKeyReference(lua_State* state, int table_index, int key_index, int revstacktop, const NoTypeCheck&) : state_(state), stack_top_(revstacktop), table_index_(table_index), key_index_(key_index)
		{
		}

		template<typename KEY>
		TableKeyReference(const LuaTable& table, const KEY& key) : state_(table.state()), stack_top_(lua_gettop(state_))
		{
			lua_type_traits<LuaRef>::push(state_, table);
			lua_type_traits<KEY>::push(state_, key);
			table_index_ = stack_top_ + 1;
			key_index_ = stack_top_ + 2;
		}
		template<typename KEY>
		TableKeyReference(const LuaRef& table,const KEY& key) : state_(table.state()), stack_top_(lua_gettop(state_))
		{
			lua_type_traits<LuaRef>::push(state_, table);
			lua_type_traits<KEY>::push(state_, key);
			table_index_ = stack_top_ + 1;
			key_index_ = stack_top_ + 2;
			if (lua_type(state_, table_index_) != LUA_TTABLE)
			{
				except::typeMismatchError(state_, "this is not table");
			}
		}

		mutable lua_State* state_;//mutable for RVO unsupported compiler
		int stack_top_;
		int table_index_;
		int key_index_;
	};

	inline std::ostream& operator<<(std::ostream& os, const TableKeyReference& ref)
	{
		ref.get<LuaRef>().dump(os);
		return os;
	}

	inline const LuaRef& toLuaRef(const LuaUserData& ref)
	{
		return static_cast<const LuaRef&>(ref);
	}
	inline LuaRef toLuaRef(const LuaTable& ref)
	{
		ref.push();
		return LuaRef(ref.state(), StackTop());
	}
	inline LuaRef toLuaRef(const TableKeyReference& ref)
	{
		return ref.get<LuaRef>();
	}

	template<typename T>
	inline bool LuaFunctionImpl<T>::setFunctionEnv(const LuaTable& env)
	{
		util::ScopedSavedStack save(state_());
		int stackIndex = pushStackIndex_(state_());
		env.push();
#if LUA_VERSION_NUM >= 502
		lua_setupvalue(state_(), stackIndex, 1);
#else
		lua_setfenv(state_(), stackIndex);
#endif
		return true;
	}
	template<typename T>
	inline bool LuaFunctionImpl<T>::setFunctionEnv(NewTable env)
	{
		return setFunctionEnv(LuaTable(state_()));
	}

	template<typename T>
	inline LuaTable LuaFunctionImpl<T>::getFunctionEnv()
	{
		util::ScopedSavedStack save(state_());
		int stackIndex = pushStackIndex_(state_());
		lua_getupvalue(state_(), stackIndex, 1);
		return LuaTable(state_(), StackTop());
	}

	/*
	inline TableKeyReference LuaRef::operator[](const LuaRef& key)
	{
		return TableKeyReference(*this, key);
	}
	inline TableKeyReference LuaRef::operator[](const char* str)
	{
		return TableKeyReference(*this, str);
	}
	inline TableKeyReference LuaRef::operator[](const std::string& str)
	{
		return TableKeyReference(*this, str);
	}
	inline TableKeyReference LuaRef::operator[](int index)
	{
		return TableKeyReference(*this, index);
	}
	*/

	template<typename T>
	bool LuaTableOrUserDataImpl<T>::setMetatable(const LuaTable& table)
	{
		lua_State* state = state_();
		if (!state)
		{
			except::typeMismatchError(state, "is nil");
			return false;
		}
		util::ScopedSavedStack save(state);
		int stackindex = pushStackIndex_(state);
		int t = lua_type(state, stackindex);
		if (t != LUA_TTABLE && t != LUA_TUSERDATA)
		{
			except::typeMismatchError(state, lua_typename(state,t) + std::string("is not table"));
			return LuaRef(state);
		}
		table.push();
		return lua_setmetatable(state, stackindex) != 0;
	}
	template<typename T>
	LuaTable LuaTableOrUserDataImpl<T>::getMetatable()const
	{
		lua_State* state = state_();
		if (!state)
		{
			except::typeMismatchError(state, "is nil");
			return LuaRef(state);
		}
		util::ScopedSavedStack save(state);
		int stackindex = pushStackIndex_(state);
		int t = lua_type(state, stackindex);
		if (t != LUA_TTABLE && t != LUA_TUSERDATA)
		{
			except::typeMismatchError(state, lua_typename(state, t)  + std::string("is not table"));
			return LuaRef(state);
		}
		lua_getmetatable(state, stackindex);
		return LuaTable(state, StackTop());
	}
	template<typename T>
	mem_fun_binder LuaTableOrUserDataImpl<T>::operator->*(const char* function_name)
	{
		push_(state_());
		return mem_fun_binder(LuaRef(state_(), StackTop()), function_name);
	}


	template<typename T> template <typename KEY>
	LuaRef LuaTableOrUserDataImpl<T>::getField(const KEY& key)const
	{
		return getField<LuaRef>(key);
	}
	template<typename T>
	std::vector<LuaRef> LuaTableOrUserDataImpl<T>::values()const
	{
		return values<LuaRef>();
	}
	template<typename T>
	std::vector<LuaRef> LuaTableOrUserDataImpl<T>::keys()const
	{
		return keys<LuaRef>();
	}
	template<typename T>
	std::map<LuaRef, LuaRef> LuaTableOrUserDataImpl<T>::map()const
	{
		return map<LuaRef, LuaRef>();
	}
	
	template<typename T> template <typename K>
	TableKeyReference LuaTableImpl<T>::operator[](const K& key)
	{
		lua_State* state = state_();
		int stack_top = lua_gettop(state);
		int stackindex = pushStackIndex_(state);
		lua_type_traits<K>::push(state, key);
		int key_index = lua_gettop(state);
		return TableKeyReference(state, stackindex, key_index, stack_top);
	}
	/*
	inline bool LuaRef::setMetatable(const LuaTable& table)
	{
		if (ref_ == LUA_REFNIL)
		{
			except::typeMismatchError(state_, "is nil");
			return false;
		}
		util::ScopedSavedStack save(state_);
		push();
		int t = lua_type(state_, -1);
		if (t != LUA_TTABLE && t != LUA_TUSERDATA)
		{
			except::typeMismatchError(state_, typeName() + std::string("is not table"));
			return false;
		}
		table.push();
		return lua_setmetatable(state_, -2) != 0;
	}

	inline LuaTable LuaRef::getMetatable()const
	{
		if (ref_ == LUA_REFNIL)
		{
			except::typeMismatchError(state_, "is nil");
			return LuaRef(state_);
		}
		util::ScopedSavedStack save(state_);
		push();
		int t = lua_type(state_, -1);
		if (t != LUA_TTABLE && t != LUA_TUSERDATA)
		{
			except::typeMismatchError(state_, typeName() + std::string("is not table"));
			return LuaRef(state_);
		}
		lua_getmetatable(state_, -1);
		return LuaRef(state_, StackTop(), NoMainCheck());
	}
	*/



	template<>
	struct lua_type_traits<TableKeyReference> {
		static int push(lua_State* l, const TableKeyReference& ref)
		{
			ref.push(l);
			return 1;
		}
	};

#ifndef KAGUYA_NO_STD_VECTOR_TO_TABLE
	template<typename T, typename A>
	struct lua_type_traits<std::vector<T, A> >
	{
		typedef std::vector<T, A> get_type;
		typedef const std::vector<T, A>& push_type;

		static bool checkType(lua_State* l, int index)
		{
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			if (table.type() != LuaRef::TYPE_TABLE) { return false; }
			std::map<LuaRef, LuaRef> values = table.map();
			for (std::map<LuaRef, LuaRef>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				if (!it->first.typeTest<size_t>() || !it->second.weakTypeTest<T>())
				{
					return false;
				}
			}
			return true;
		}
		static bool strictCheckType(lua_State* l, int index)
		{
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			if (table.type() != LuaRef::TYPE_TABLE) { return false; }
			std::map<LuaRef, LuaRef> values = table.map();
			for (std::map<LuaRef, LuaRef>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				if (!it->first.typeTest<size_t>() || !it->second.typeTest<T>())
				{
					return false;
				}
			}
			return true;
		}

		static get_type get(lua_State* l, int index)
		{
			get_type result;
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			std::vector<LuaRef> values = table.values();
			result.reserve(values.size());
			for (std::vector<LuaRef>::iterator it = values.begin(); it != values.end(); ++it)
			{
				result.push_back(it->get<T>());
			}
			return result;
		}
		static int push(lua_State* l, push_type v)
		{
			LuaRef table(l, NewTable(int(v.size()), 0));

			int count = 1;//array is 1 origin in Lua
			for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it)
			{
				table.setField(count++, *it);
			}
			table.push(l);
			return 1;
		}
	};
#endif

#ifndef KAGUYA_NO_STD_MAP_TO_TABLE
	template<typename K, typename V, typename C, typename A>
	struct lua_type_traits<std::map<K, V, C, A> >
	{
		typedef std::map<K, V, C, A> get_type;
		typedef const std::map<K, V, C, A>& push_type;

		static bool checkType(lua_State* l, int index)
		{
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			if (table.type() != LuaRef::TYPE_TABLE) { return false; }
			std::map<LuaRef, LuaRef> values = table.map();
			for (std::map<LuaRef, LuaRef>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				if (!it->first.typeTest<K>() || !it->second.weakTypeTest<V>())
				{
					return false;
				}
			}
			return true;
		}
		static bool strictCheckType(lua_State* l, int index)
		{
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			if (table.type() != LuaRef::TYPE_TABLE) { return false; }
			std::map<LuaRef, LuaRef> values = table.map();
			for (std::map<LuaRef, LuaRef>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				if (!it->first.typeTest<K>() || !it->second.typeTest<V>())
				{
					return false;
				}
			}
			return true;
		}

		static get_type get(lua_State* l, int index)
		{
			get_type result;
			LuaRef table = lua_type_traits<LuaRef>::get(l, index);
			std::map<LuaRef, LuaRef> values = table.map();
			for (std::map<LuaRef, LuaRef>::const_iterator it = values.begin(); it != values.end(); ++it)
			{
				result[it->first.get<K>()] = it->second.get<V>();
			}
			return result;
		}
		static int push(lua_State* l, push_type v)
		{
			LuaRef table(l, NewTable(0, int(v.size())));
			for (typename std::map<K, V>::const_iterator it = v.begin(); it != v.end(); ++it)
			{
				table.setField(it->first, it->second);
			}
			table.push(l);
			return 1;
		}
	};
#endif
}
