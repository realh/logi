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

class DvbtTuningIterator : public TuningIterator
{
private:
    guint32 first_, last_, step_, current_, bandwidth_;
public:
    /**
     * DvbtTuningIterator:
     * Defaults are for UK (Freeview).
     * @first_f:    First frequency.
     * @last_f:     Last frequency.
     * FIXME: last_f should be 67, not 37 (shortcut for testing)
     * @step:       Gap between frequencies.
     * @bandwidth:  Bandwidth (default is same as step).
     */
    DvbtTuningIterator(guint32 first_f = (21 * 8 + 306) * 1000000,
            guint32 last_f = (37 * 8 + 306) * 1000000, guint32 step = 8000000,
            guint32 bandwidth = 0);

    // Default copy/move are OK

    std::shared_ptr<TuningProperties> next() override;

    void reset() override;
};

}
