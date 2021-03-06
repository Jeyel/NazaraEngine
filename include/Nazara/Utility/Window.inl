// Copyright (C) 2015 Jérôme Leclercq
// This file is part of the "Nazara Engine - Core module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Utility/Window.hpp>
#include <Nazara/Core/ErrorFlags.hpp>
#include <Nazara/Utility/Debug.hpp>

namespace Nz
{
	/*!
	* \class Nz::Window
	*/

	inline Window::Window() :
	#if NAZARA_UTILITY_THREADED_WINDOW
	m_impl(nullptr),
	m_eventListener(true),
	m_waitForEvent(false)
	#else
	m_impl(nullptr)
	#endif
	{
	}

	inline Window::Window(VideoMode mode, const String& title, UInt32 style) :
	#if NAZARA_UTILITY_THREADED_WINDOW
	m_impl(nullptr),
	m_eventListener(true),
	m_waitForEvent(false)
	#else
	m_impl(nullptr)
	#endif
	{
		ErrorFlags flags(ErrorFlag_ThrowException, true);
		Create(mode, title, style);
	}

	inline Window::Window(WindowHandle handle) :
	#if NAZARA_UTILITY_THREADED_WINDOW
	m_impl(nullptr),
	m_eventListener(true),
	m_waitForEvent(false)
	#else
	m_impl(nullptr)
	#endif
	{
		ErrorFlags flags(ErrorFlag_ThrowException, true);
		Create(handle);
	}

	/*!
	* \brief Constructs a Window object by moving another one
	*/
	inline Window::Window(Window&& window) noexcept :
	m_impl(window.m_impl),
	m_events(std::move(window.m_events)),
	#if NAZARA_UTILITY_THREADED_WINDOW
	m_eventCondition(std::move(window.m_eventCondition)),
	m_eventMutex(std::move(window.m_eventMutex)),
	m_eventConditionMutex(std::move(window.m_eventConditionMutex)),
	m_eventListener(window.m_eventListener),
	m_waitForEvent(window.m_waitForEvent),
	#endif
	m_closed(window.m_closed),
	m_ownsWindow(window.m_ownsWindow)
	{
		window.m_impl = nullptr;
	}

	inline Window::~Window()
	{
		Destroy();
	}

	inline void Window::Close()
	{
		m_closed = true; // The window will be closed at the next non-const IsOpen() call
	}

	inline bool Window::IsOpen(bool checkClosed)
	{
		if (!m_impl)
			return false;

		if (checkClosed && m_closed)
		{
			Destroy();
			return false;
		}

		return true;
	}

	inline bool Window::IsOpen() const
	{
		return m_impl != nullptr;
	}

	inline bool Window::IsValid() const
	{
		return m_impl != nullptr;
	}

	inline void Window::PushEvent(const WindowEvent& event)
	{
		#if NAZARA_UTILITY_THREADED_WINDOW
		m_eventMutex.Lock();
		#endif

		m_events.push(event);
		if (event.type == WindowEventType_Resized)
			OnWindowResized();

		#if NAZARA_UTILITY_THREADED_WINDOW
		m_eventMutex.Unlock();

		if (m_waitForEvent)
		{
			m_eventConditionMutex.Lock();
			m_eventCondition.Signal();
			m_eventConditionMutex.Unlock();
		}
		#endif
	}

	/*!
	* \brief Moves a window to another window object
	* \return A reference to the object
	*/
	inline Window& Window::operator=(Window&& window)
	{
		Destroy();

		m_closed     = window.m_closed; 
		m_impl       = window.m_impl;
		m_events     = std::move(window.m_events);
		m_ownsWindow = window.m_ownsWindow;

		window.m_impl = nullptr;

		#if NAZARA_UTILITY_THREADED_WINDOW
		m_eventCondition      = std::move(window.m_eventCondition);
		m_eventMutex          = std::move(window.m_eventMutex);
		m_eventConditionMutex = std::move(window.m_eventConditionMutex);
		m_eventListener       = window.m_eventListener;
		m_waitForEvent        = window.m_waitForEvent;
		#endif

		return *this;
	}
}

#include <Nazara/Utility/DebugOff.hpp>
