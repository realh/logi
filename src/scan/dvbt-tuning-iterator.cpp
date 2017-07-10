/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

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

#include "dvbt-tuning-iterator.h"

namespace logi
{

DvbtTuningIterator::DvbtTuningIterator(guint32 first_f, guint32 last_f,
        guint32 step, guint32 bandwidth) :
    first_{first_f}, last_{last_f}, step_{step}, current_{first_f},
    bandwidth_{bandwidth ? bandwidth : step}
{}

std::shared_ptr<TuningProperties> DvbtTuningIterator::next()
{
    if (current_ > last_)
        return nullptr;

    TuningProperties *props = new TuningProperties(
        {
            {DTV_DELIVERY_SYSTEM, SYS_DVBT},
            {DTV_FREQUENCY, current_},
            {DTV_BANDWIDTH_HZ, bandwidth_},
            {DTV_TUNE, 1},
        });

    current_ += step_;

    return std::shared_ptr<TuningProperties>(props);
}

void DvbtTuningIterator::reset()
{
    current_ = first_;
}

}
