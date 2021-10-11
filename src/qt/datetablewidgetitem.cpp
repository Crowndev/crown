#include <qt/datetablewidgetitem.h>
#include <util/time.h>

DateTableWidgetItem::DateTableWidgetItem(int64_t date)
    : QTableWidgetItem(QString::fromStdString(DurationToDHMS(date)))
{
    this->date = date;
}

bool DateTableWidgetItem::operator< ( const QTableWidgetItem & other ) const
{
    const DateTableWidgetItem &other_cast = static_cast<const DateTableWidgetItem &>(other);
    return date < other_cast.date;
}
