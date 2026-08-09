#include <stdlib.h>
#include <assert.h>

#include "config_extra.h"

void
colors()
{
    /* Invalid red */
    {
        char *color = "invalid,invalid,invalid";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Invalid green */
    {
        char *color = "12,Invalid,invalid";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Invalid blue */
    {
        char *color = "12,12,invalid";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Red overflows */
    {
        char *color = "9999999999999999999999,9999999999999999999999,9999999999999999999999";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Blue overflows */
    {
        char *color = "9,9999999999999999999999,9999999999999999999999";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Green overflows */
    {
        char *color = "9,9,9999999999999999999999";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Red < 0 */
    {
        char *color = "-1,-1,-1";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Blue < 0 */
    {
        char *color = "1,-1,-1";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Green < 0 */
    {
        char *color = "1,1,-1";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Red > 255 */
    {
        char *color = "256,256,256";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Green > 255 */
    {
        char *color = "254,256,256";
        assert(!extract_color(NULL, color, NULL, 0));
    }

    /* Blue > 255 */
    {
        char *color = "254,254,256";
        assert(!extract_color(NULL, color, NULL, 0));
    }
}

void
height()
{
    /* Invalid height */
    {
        char *value = "invalid";
        assert(!set_height(NULL, value, 0));
    }

    /* Height underflows */
    {
        char *value = "-99999999999999999999999999999999999999999999";
        assert(!set_height(NULL, value, 0));
    }

    /* Height overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_height(NULL, value, 0));
    }
}

void
width()
{
    /* Invalid width */
    {
        char *value = "invalid";
        assert(!set_width(NULL, value, 0));
    }

    /* Width underflows */
    {
        char *value = "-99999999999999999999999999999999999999999999";
        assert(!set_width(NULL, value, 0));
    }

    /* Width overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_width(NULL, value, 0));
    }
}

void
position()
{
    /* Invalid position */
    char *value = "invalid";
    assert(!set_pos(NULL, value, 0));
}

void
opacity()
{
    /* Invalid opacity */
    {
        char *value = "invalid";
        assert(!set_opacity(NULL, value, 0));
    }

    /* Opacity underflows */
    {
        char *value = "-99999999999999999999999999999999999999999999";
        assert(!set_opacity(NULL, value, 0));
    }

    /* Opacity overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_opacity(NULL, value, 0));
    }

    /* Opacity < 0 */
    {
        char *value = "-1";
        assert(!set_opacity(NULL, value, 0));
    }

    /* Opacity > 1 */
    {
        char *value = "2";
        assert(!set_opacity(NULL, value, 0));
    }
}

void
margin()
{
    /* Invalid margin */
    {
        char *value = "invalid";
        assert(!set_margin(NULL, value, 0));
    }

    /* Margin underflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_margin(NULL, value, 0));
    }

    /* Margin overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_margin(NULL, value, 0));
    }
}

void
border_width()
{
    /* Invalid width */
    {
        char *value = "invalid";
        assert(!set_border_width(NULL, value, 0));
    }

    /* Border width underflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_border_width(NULL, value, 0));
    }

    /* Border width overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_border_width(NULL, value, 0));
    }
}

void
border_opacity()
{
    /* Invalid border opacity */
    {
        char *value = "invalid";
        assert(!set_border_opacity(NULL, value, 0));
    }

    /* Border opacity underflows */
    {
        char *value = "-99999999999999999999999999999999999999999999";
        assert(!set_border_opacity(NULL, value, 0));
    }

    /* Border opacity overflows */
    {
        char *value = "99999999999999999999999999999999999999999999";
        assert(!set_border_opacity(NULL, value, 0));
    }

    /* Border opacity < 0 */
    {
        char *value = "-1";
        assert(!set_border_opacity(NULL, value, 0));
    }

    /* Border opacity > 1 */
    {
        char *value = "2";
        assert(!set_border_opacity(NULL, value, 0));
    }
}

void
layer()
{
    /* Invalid layer */
    char *value = "invalid";
    assert(!set_layer(NULL, value, 0));
}

int
main(void)
{
    colors();
    height();
    width();
    position();
    opacity();
    margin();
    border_width();
    border_opacity();
    layer();

    return EXIT_SUCCESS;
}
