// ISoundPlayer.hh
//
// Copyright (C) 2002, 2003, 2004, 2005, 2006 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$
//

#ifndef ISOUNDPLAYER_HH
#define ISOUNDPLAYER_HH


class ISoundPlayer
{
public:
  enum Sound
  {
    SOUND_BREAK_PRELUDE = 0,
    SOUND_BREAK_IGNORED,
    SOUND_REST_BREAK_STARTED,
    SOUND_REST_BREAK_ENDED,
    SOUND_MICRO_BREAK_STARTED,
    SOUND_MICRO_BREAK_ENDED,
    SOUND_DAILY_LIMIT,
    SOUND_EXERCISE_ENDED,
    SOUND_EXERCISES_ENDED
  };

  virtual ~ISoundPlayer() {}
  
  //! Plays sound, returns immediately.
  virtual void play_sound(Sound snd) = 0;
};

#endif // ISOUNDPLAYER_HH