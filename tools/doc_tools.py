import os


def word_to_pdf(docx_path: str, pdf_path: str = None) -> str:
    from docx import Document
    from fpdf import FPDF

    if pdf_path is None:
        pdf_path = os.path.splitext(docx_path)[0] + ".pdf"

    doc = Document(docx_path)
    pdf = FPDF()
    pdf.add_page()
    pdf.set_auto_page_break(auto=True, margin=15)
    
    # Try to use a Unicode-capable font if available, else fallback to Helvetica
    font_path = os.path.join(os.path.dirname(__file__), "..", "DejaVuSans.ttf")
    if os.path.exists(font_path):
        pdf.add_font("DejaVu", "", font_path, uni=True)
        pdf.set_font("DejaVu", "", 12)
    else:
        pdf.set_font("Helvetica", "", 12)

    for para in doc.paragraphs:
        text = para.text.strip()
        if text:
            # Multi-cell handles line wrapping
            try:
                pdf.multi_cell(0, 10, text)
            except:
                # Fallback for characters not in current font
                pdf.multi_cell(0, 10, text.encode("latin-1", "replace").decode("latin-1"))
        else:
            pdf.ln(5)

    pdf.output(pdf_path)
    return pdf_path


def pdf_to_word(pdf_path: str, docx_path: str = None) -> str:
    from docx import Document
    from pypdf import PdfReader

    if docx_path is None:
        docx_path = os.path.splitext(pdf_path)[0] + ".docx"

    doc = Document()
    reader = PdfReader(pdf_path)
    
    for i, page in enumerate(reader.pages):
        text = page.extract_text()
        if text:
            for line in text.splitlines():
                if line.strip():
                    doc.add_paragraph(line.strip())
        if i < len(reader.pages) - 1:
            doc.add_page_break()

    doc.save(docx_path)
    return docx_path


def image_to_pdf(image_paths: list, pdf_path: str = "output.pdf") -> str:
    import img2pdf
    with open(pdf_path, "wb") as f:
        f.write(img2pdf.convert(image_paths))
    return pdf_path


def split_pdf(pdf_path: str, output_dir: str = None) -> list:
    import pikepdf

    if output_dir is None:
        output_dir = os.path.dirname(pdf_path) or "."
    os.makedirs(output_dir, exist_ok=True)

    base = os.path.splitext(os.path.basename(pdf_path))[0]
    paths = []

    with pikepdf.open(pdf_path) as pdf:
        for i, page in enumerate(pdf.pages):
            out = pikepdf.Pdf.new()
            out.pages.append(page)
            out_path = os.path.join(output_dir, f"{base}_page_{i+1}.pdf")
            out.save(out_path)
            out.close()
            paths.append(out_path)

    return paths


def merge_pdfs(pdf_list: list, output_path: str = "merged.pdf") -> str:
    from tools.pdf import merge_documents
    merge_documents(pdf_list, output_path)
    return output_path


def text_to_pdf(text: str, pdf_path: str = "output.pdf") -> str:
    from fpdf import FPDF
    pdf = FPDF()
    pdf.add_page()
    pdf.set_auto_page_break(auto=True, margin=15)
    
    font_path = os.path.join(os.path.dirname(__file__), "..", "DejaVuSans.ttf")
    if os.path.exists(font_path):
        pdf.add_font("DejaVu", "", font_path, uni=True)
        pdf.set_font("DejaVu", "", 12)
    else:
        pdf.set_font("Helvetica", "", 12)

    for line in text.splitlines():
        try:
            pdf.multi_cell(0, 10, line)
        except:
            pdf.multi_cell(0, 10, line.encode("latin-1", "replace").decode("latin-1"))
    pdf.output(pdf_path)
    return pdf_path
