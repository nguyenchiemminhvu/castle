#include "sample_support.h"

#include "castle/iterator/tags.h"

// Scenario: using iterator category tags for dispatch in generic embedded algorithms.
int main()
{
    castle::input_iterator_tag input;
    castle::forward_iterator_tag forward;
    castle::bidirectional_iterator_tag bidi;
    castle::random_access_iterator_tag random;
    (void)input;
    (void)forward;
    (void)bidi;
    (void)random;
    return 0;
}
