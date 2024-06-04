#include <gtk/gtk.h>
#include <poppler.h>

// Models the size of a PDF page and the scaling factor to fit it.
typedef struct {
  double height;
  double width;
  double scaling_factor;
} PageSize;

// Models a PDF document loaded ready for rendering the current page.
typedef struct {
  PopplerDocument *document;
  PopplerPage *current;
  int currentPageIndex;
  int totalPages;
} Pdf;

// Renders the current page of the PDF to the given Cairo context.
void pdf_render_current_page(Pdf *pdf, cairo_t *cairo) {
  poppler_page_render(pdf->current, cairo);
}

// Returns the page size scaled with a factor to fit the PDF page to the given width.
PageSize pdf_scale_to_width(Pdf *pdf, double width) {
  double pwidth, pheight;
  poppler_page_get_size(pdf->current, &pwidth, &pheight);
  double sf = width / pwidth;
  PageSize ps = {pheight * sf, width, sf};
  return ps;
}

// Opens a PDF file and fills the Pdf object provided.
void pdf_open(Pdf *pdf, const char *uri) {
  GError *gerror;

  PopplerDocument *document = poppler_document_new_from_file(uri, NULL, &gerror);
  if (!document) {
    g_error("Error opening PDF document %s", uri);
  }

  pdf->document = document;
  pdf->totalPages = poppler_document_get_n_pages(document);
  pdf->current = poppler_document_get_page(document, 0);
  pdf->currentPageIndex = 0;
}

// Moves the current page of the PDF by the given increment. Returns true if the
// current page changed.
bool pdf_advance_current_page(Pdf *pdf, int increment) {
  int previous = pdf->currentPageIndex;

  if (increment > 0) {
    pdf->currentPageIndex = MIN(pdf->totalPages - 1, pdf->currentPageIndex + increment);
  } else {
    pdf->currentPageIndex = MAX(0, pdf->currentPageIndex + increment);
  }

  if (previous == pdf->currentPageIndex) {
    return false;
  }

  pdf->current = poppler_document_get_page(pdf->document, pdf->currentPageIndex);
  return true;
}

// Models the UI state of the PDF viewer.
typedef struct {
  Pdf *doc_model;
  GtkDrawingArea *drawing_area;
  GtkScrolledWindow *scrolled_window;
  GtkAdjustment *vadjustment;
  bool mustScrollToTheBottom;
} PdfView;

// Callback for the draw signal of the drawing area to render a Pdf (assuming
// the PdfView is passed as user data).
static void pdf_view_draw_content(GtkDrawingArea *area, cairo_t *cairo, int width, int height, gpointer user_data) {

  // Clip and scale the Cairo context to the visible area
  cairo_rectangle(cairo, 0, 0, width, height);
  cairo_clip(cairo);

  PdfView *view = (PdfView *)user_data;
  PageSize ps = pdf_scale_to_width(view->doc_model, width);
  cairo_scale(cairo, ps.scaling_factor, ps.scaling_factor);

  pdf_render_current_page(view->doc_model, cairo);

  // Places the scroll manually.
  gtk_widget_set_size_request(GTK_WIDGET(view->drawing_area), ps.width, ps.height);
  double adjust = gtk_adjustment_get_lower(view->vadjustment);
  if (view->mustScrollToTheBottom) {
    adjust = gtk_adjustment_get_upper(view->vadjustment);
  }
  gtk_adjustment_set_value(view->vadjustment, adjust);
  g_print("Draw: PDF size: %f x %f SF: %f\n", ps.width, ps.height, ps.scaling_factor);
  g_print("Draw: Window size: %d x %d\n", width, height);
}

// Callback for the edge-overshot signal of the scrolled window to advance the current page.
static void pdf_view_on_overshot(GtkScrolledWindow *scrolled_window, GtkPositionType pos, gpointer user_data) {
  PdfView *view = (PdfView *)user_data;

  int increment = pos == GTK_POS_TOP ? -1 : 1;

  if (pdf_advance_current_page(view->doc_model, increment)) {
    view->mustScrollToTheBottom = pos == GTK_POS_TOP;
    GtkWidget *drawing_area = GTK_WIDGET(view->drawing_area);
    gtk_widget_queue_draw(drawing_area);
  }
}

// Activates the PDF viewer by creating the drawing area and scrolled window.
void pdf_view_activate(PdfView *view) {
  view->drawing_area = GTK_DRAWING_AREA(gtk_drawing_area_new());
  gtk_widget_set_vexpand(GTK_WIDGET(view->drawing_area), TRUE);
  gtk_drawing_area_set_draw_func(view->drawing_area, pdf_view_draw_content, view, NULL);

  view->scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view->scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_halign(GTK_WIDGET(view->scrolled_window), GTK_ALIGN_FILL);
  gtk_widget_set_valign(GTK_WIDGET(view->scrolled_window), GTK_ALIGN_FILL);
  gtk_scrolled_window_set_child(view->scrolled_window, GTK_WIDGET(view->drawing_area));
  g_signal_connect(GTK_WIDGET(view->scrolled_window), "edge-overshot", G_CALLBACK(pdf_view_on_overshot), view);

  view->vadjustment = gtk_scrolled_window_get_vadjustment(view->scrolled_window);

  view->mustScrollToTheBottom = false;
}

// Callback for the window size changed signal to fit the drawing area to the window width.
static void pdf_view_on_window_size_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
  GtkWindow *window = GTK_WINDOW(object);
  gint width, height;
  PdfView *view = (PdfView *)user_data;

  // Get the current size from the properties
  gtk_window_get_default_size(window, &width, &height);

  // fit width. Will need the scaling factor in the drawing function.
  PageSize ps = pdf_scale_to_width(view->doc_model, width);

  gtk_widget_set_size_request(GTK_WIDGET(view->drawing_area), ps.width, ps.height);
  g_print("ON size change: PDF size: %f x %f SF: %f\n", ps.width, ps.height, ps.scaling_factor);
  g_print("ON size change: Window size: %d x %d\n", width, height);
}

// Returns the top-level widget of the PDF viewer, because I'm lazy to make the PdfView a GObject/GtkWidget subclass.
GtkWidget *pdf_view_get_widget(PdfView *view) {
  return GTK_WIDGET(view->scrolled_window);
}

// Callback for the activate signal of the application to create the window and the PDF viewer and other UI elements.
static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *viewport;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Adobe Acrobat GTK4");
  PdfView *view = (PdfView *)user_data;
  pdf_view_activate(view);
  g_signal_connect(window, "notify::default-height", G_CALLBACK(pdf_view_on_window_size_changed), view);
  g_signal_connect(window, "notify::default-width", G_CALLBACK(pdf_view_on_window_size_changed), view);

  // Icon as a smiley PNG image (replace with your image file)
  image = gtk_image_new_from_file("smile.jpg");

  // Hello World Label
  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), "<span size=\"large\"> I wish this was Flutter </span>");
  gtk_widget_set_size_request(label, -1, 32);

  // Box for vertical layout
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_window_set_child(GTK_WINDOW(window), box);

  // Add elements to the box
  gtk_box_append(GTK_BOX(box), image);
  gtk_widget_set_size_request(image, -1, 128);
  gtk_box_append(GTK_BOX(box), label);
  gtk_box_append(GTK_BOX(box), pdf_view_get_widget(view));

  gtk_window_maximize(GTK_WINDOW(window));
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
  GtkApplication *app;
  int status;

  Pdf pdf;
  // TODO: Adjust to your PDF file path.
  pdf_open(&pdf, "file:///home/cnihelton/Dev/3_Explore/pocs/pdfgtk4/CartilhaCCBrasil.pdf");

  PdfView view;
  view.doc_model = &pdf;

  app = gtk_application_new("org.example.pdfgtk4", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), &view);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
