#include "events.hpp"
#include "mouse.hpp"
#include "preferences.hpp"

#include "SDL.h"

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace events
{

namespace {
	int disallow_resize = 0;
}

resize_lock::resize_lock()
{
	++disallow_resize;
}

resize_lock::~resize_lock()
{
	--disallow_resize;
}

std::vector<handler*> event_handlers;

handler::handler() : unicode_(SDL_EnableUNICODE(1))
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
	event_handlers.push_back(this);
}

handler::~handler()
{
	assert(!event_handlers.empty());
	if(event_handlers.back() == this) {
		event_handlers.pop_back();
	} else {
		event_handlers.erase(std::find(event_handlers.begin(),
		                               event_handlers.end(),this));
	}

	SDL_EnableUNICODE(unicode_);
}

void pump()
{
	SDL_PumpEvents();

	static std::pair<int,int> resize_dimensions(0,0);

	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		//events may cause more event handlers to be added and/or removed,
		//so we must use indexes instead of iterators here.
		for(size_t i1 = 0, i2 = event_handlers.size(); i1 != i2; ++i1) {
			assert(i1 < event_handlers.size());
			event_handlers[i1]->handle_event(event);
		}

		switch(event.type) {
			case SDL_VIDEORESIZE: {
				const SDL_ResizeEvent* const resize
				              = reinterpret_cast<SDL_ResizeEvent*>(&event);

				if(resize->w < 1024 || resize->h < 768) {
					resize_dimensions.first = 0;
					resize_dimensions.second = 0;
				} else {
					resize_dimensions.first = resize->w;
					resize_dimensions.second = resize->h;
				}

				break;
			}

			//mouse wheel support
			case SDL_MOUSEBUTTONDOWN: {
				if(event.button.button == 4) {
					gui::scroll_dec();
				} else if(event.button.button == 5) {
					gui::scroll_inc();
				}

				break;
			}

			case SDL_QUIT: {
				throw CVideo::quit();
			}
		}
	}

	if(resize_dimensions.first > 0 && disallow_resize == 0) {
		preferences::set_resolution(resize_dimensions);
		resize_dimensions.first = 0;
		resize_dimensions.second = 0;
	}
}

}
