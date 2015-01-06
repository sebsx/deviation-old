/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

#include "../common/_chantest_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static const char *_channum_cb(guiObject_t *obj, const void *data);
static const char *_title_cb(guiObject_t *obj, const void *data);
static const char *_page_cb(guiObject_t *obj, const void *data);
static s8 current_page = 0; // bug fix

static void draw_chan(long disp, int row, int y)
{
    int x = disp%2 ? 63 : 0;
    int idx = disp%2 ? 2*row + 1 : 2*row;
    int height;
    long ch = get_channel_idx(disp);
    if (cp->type == MONITOR_RAWINPUT) {
        labelDesc.font = DEFAULT_FONT.font;  // Could be translated to other languages, hence using 12normal
        height = LINE_HEIGHT;
    } else {
        labelDesc.font = MICRO_FONT.font;  // only digits, can use smaller font to show more channels
        height = 7;
    }
    GUI_CreateLabelBox(&gui->chan[idx], x, y,
        0, height, &labelDesc, _channum_cb, NULL, (void *)ch);
    GUI_CreateLabelBox(&gui->value[idx], x+37, y,
        23, height, &MICRO_FONT, value_cb, NULL, (void *)disp);
    GUI_CreateBarGraph(&gui->bar[idx], x, y + height,
        59, 4, -125, 125, TRIM_HORIZONTAL, showchan_cb, (void *)disp);

    // Bug fix: the labelDesc is shared in many pages, must reset it to DEFAULT_FONT after the page is drawn
    // Otherwise, page in other language will not display as only the DEFAULT_FONT supports multi-lang
    labelDesc.font = DEFAULT_FONT.font;
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    switch(col) {
        case ITEM_GRAPH:  return (guiObject_t *)&gui->bar[2*relrow];
        case ITEM_GRAPH2: return (guiObject_t *)&gui->bar[2*relrow+1];
        case ITEM_VALUE2: return (guiObject_t *)&gui->value[2*relrow+1];
        default:          return (guiObject_t *)&gui->value[2*relrow];
    }
}
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    draw_chan(absrow*2, relrow, y);
    if(absrow*2+1 < cp->num_bars)
        draw_chan(absrow*2+1, relrow, y);
    return 0;
}

static void _show_bar_page(u8 num_bars)
{
    current_page = 0;
    cp->num_bars = num_bars;
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    int view_height = (cp->type == MONITOR_RAWINPUT)
                      ? (LINE_HEIGHT + 5)   // can only show 3 rows: (12 + 5) x 3
                      : 12;  // can only show 4 rows: (7 + 5) x 4
    labelDesc.style = LABEL_UNDERLINE;
    GUI_CreateLabelBox(&gui->title, 0 , 0, LCD_WIDTH, LINE_HEIGHT, &labelDesc, _title_cb, NULL, (void *)NULL);
    labelDesc.style = LABEL_LEFTCENTER;  // bug fix: must initialize to avoid unpredictable drawing
    //GUI_CreateRect(&gui->rect, 0, HEADER_HEIGHT - 1, LCD_WIDTH, 1, &labelDesc);

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         view_height, (num_bars + 1)/2, row_cb, getobj_cb, NULL, NULL);
    u8 w = 10;
    GUI_CreateLabelBox(&gui->page, LCD_WIDTH -w, 0, w, 7, &DEFAULT_FONT, _page_cb, NULL, NULL);
}

void PAGE_ChantestInit(int page)
{
    (void)channum_cb; // remove compile warning as this method is not used here
    (void)okcancel_cb;
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    PAGE_RemoveAllObjects();
    cp->return_page = NULL;
    if (page > 0)
        cp->return_val = page;
    int j = 0;
    if (cp->type == MONITOR_RAWINPUT) {
        for (int i = 0; i < NUM_INPUTS; i++) {
          if (!(Transmitter.ignore_src & (1 << (i+1)))) {
              j += 1;
            }
          }
        _show_bar_page(j + (Model.num_ppmin & 0x3f));
    } else {
        cp->type = MONITOR_MIXEROUTPUT;// cp->type may not be initialized yet, so do it here
        for (int i = 0; i < NUM_CHANNELS; i += 1) {
            if (Model.templates[i] != MIXERTEMPLATE_NONE) {
                j += 1;
            }
        }
        _show_bar_page(j);
    }
}

void PAGE_ChantestModal(void(*return_page)(int page), int page)
{
    cp->type = MONITOR_MIXEROUTPUT;
    PAGE_ChantestInit(page);
    cp->return_page = return_page;
    cp->return_val = page;
}

static void _navigate_pages(s8 direction)
{
    if ((direction == -1 && cp->type == MONITOR_RAWINPUT) ||
            (direction == 1 && cp->type == MONITOR_MIXEROUTPUT)) {
        cp->type = cp->type == MONITOR_RAWINPUT?MONITOR_MIXEROUTPUT: MONITOR_RAWINPUT;
        PAGE_ChantestInit(0);
    }
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            labelDesc.font = DEFAULT_FONT.font;
            if (cp->return_val == 2) // indicating this page is entered from calibration page, so back to its parent page
                PAGE_ChangeByID(PAGEID_TXCFG, -1);
            else
                PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            _navigate_pages(1);
        }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
            _navigate_pages(-1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static const char *_channum_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    INPUT_SourceName(tempstring, ch+1);
    return tempstring;
}

static const char *_title_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (cp->type == MONITOR_RAWINPUT) {
        tempstring_cpy((const char *)_tr("Raw input"));
    } else {
        tempstring_cpy((const char *)_tr("Mixer output"));
    }
    return tempstring;
}

static const char *_page_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    tempstring_cpy((const char *)"->");  //this is actually used as an icon don't translate
    if (cp->type == MONITOR_RAWINPUT) {
        tempstring_cpy((const char *)"<-");
    }
    return tempstring;
}
void _handle_button_test() {}

static inline guiObject_t *_get_obj(int chan, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, chan / 2, chan % 2 ? objid + 2 : objid);
}

static inline int get_channel_idx(int ch) {
  return _get_channel_idx(ch);
}
