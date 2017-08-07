#pragma once

/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016-2017 Tony Houghton <h@realh.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "tuning-iterator.h"

namespace logi
{

class FreesatTuningIterator : public TuningIterator
{
public:
    FreesatTuningIterator() : iter_(0) {}

    // Default copy/move are OK

    std::shared_ptr<TuningProperties> next() override;

    void reset() override;
private:
    unsigned iter_;

    static char const *presets_[];
};

}
