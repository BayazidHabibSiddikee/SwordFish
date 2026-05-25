import os


def docx_to_pdf(docx_path: str, pdf_path: str = None) -> str:
    from docx import Document
    from fpdf import FPDF

    if pdf_path is None:
        pdf_path = os.path.splitext(docx_path)[0] + ".pdf"

    doc = Document(docx_path)
    pdf = FPDF()
    pdf.add_page()
    pdf.set_auto_page_break(auto=True, margin=15)
    pdf.set_font("Courier", "", 10)

    for para in doc.paragraphs:
        text = para.text.strip()
        if text:
            pdf.multi_cell(0, 6, text)
        else:
            pdf.ln(4)

    pdf.output(pdf_path)
    return pdf_path


def pdf_to_docx(pdf_path: str, docx_path: str = None) -> str:
    from docx import Document
    import pikepdf

    if docx_path is None:
        docx_path = os.path.splitext(pdf_path)[0] + ".docx"

    doc = Document()
    with pikepdf.open(pdf_path) as pdf:
        for page in pdf.pages:
            texts = []
            if "/Contents" in page:
                try:
                    stream = page["/Contents"].read_bytes()
                    text = stream.decode("utf-8", errors="replace")
                    texts.append(text)
                except Exception:
                    pass
            for t in texts:
                for line in t.splitlines():
                    if line.strip():
                        doc.add_paragraph(line.strip())
            doc.add_page_break()

    doc.save(docx_path)
    return docx_path


def xlsx_to_pdf(xlsx_path: str, pdf_path: str = None) -> str:
    import openpyxl
    from fpdf import FPDF

    if pdf_path is None:
        pdf_path = os.path.splitext(xlsx_path)[0] + ".pdf"

    wb = openpyxl.load_workbook(xlsx_path)
    ws = wb.active

    pdf = FPDF()
    pdf.add_page()
    pdf.set_font("Courier", "", 8)

    col_width = max(10, 180 // max(ws.max_column, 1))

    for row in ws.iter_rows(values_only=True):
        line = " | ".join(str(cell) if cell is not None else "" for cell in row)
        pdf.multi_cell(0, 5, line)

    pdf.output(pdf_path)
    return pdf_path


def pdf_to_xlsx(pdf_path: str, xlsx_path: str = None) -> str:
    import openpyxl
    import pikepdf

    if xlsx_path is None:
        xlsx_path = os.path.splitext(pdf_path)[0] + ".xlsx"

    wb = openpyxl.Workbook()
    ws = wb.active

    with pikepdf.open(pdf_path) as pdf:
        row = 1
        for page in pdf.pages:
            if "/Contents" in page:
                try:
                    stream = page["/Contents"].read_bytes()
                    text = stream.decode("utf-8", errors="replace")
                    for line in text.splitlines():
                        if line.strip():
                            ws.cell(row=row, column=1, value=line.strip())
                            row += 1
                except Exception:
                    pass

    wb.save(xlsx_path)
    return xlsx_path


def pptx_to_pdf(pptx_path: str, pdf_path: str = None) -> str:
    from pptx import Presentation
    from fpdf import FPDF

    if pdf_path is None:
        pdf_path = os.path.splitext(pptx_path)[0] + ".pdf"

    prs = Presentation(pptx_path)
    pdf = FPDF()
    pdf.set_font("Courier", "", 10)

    for slide_num, slide in enumerate(prs.slides, 1):
        pdf.add_page()
        pdf.set_font("Courier", "B", 14)
        pdf.cell(0, 10, f"Slide {slide_num}", ln=True)
        pdf.set_font("Courier", "", 10)
        for shape in slide.shapes:
            if shape.has_text_frame:
                for para in shape.text_frame.paragraphs:
                    text = para.text.strip()
                    if text:
                        pdf.multi_cell(0, 6, text)
                    else:
                        pdf.ln(3)

    pdf.output(pdf_path)
    return pdf_path


def csv_to_xlsx(csv_path: str, xlsx_path: str = None, delimiter: str = ",") -> str:
    import csv
    import openpyxl

    if xlsx_path is None:
        xlsx_path = os.path.splitext(csv_path)[0] + ".xlsx"

    wb = openpyxl.Workbook()
    ws = wb.active

    with open(csv_path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=delimiter)
        for row_idx, row in enumerate(reader, 1):
            for col_idx, val in enumerate(row, 1):
                ws.cell(row=row_idx, column=col_idx, value=val)

    wb.save(xlsx_path)
    return xlsx_path


def xlsx_to_csv(xlsx_path: str, csv_path: str = None) -> str:
    import csv
    import openpyxl

    if csv_path is None:
        csv_path = os.path.splitext(xlsx_path)[0] + ".csv"

    wb = openpyxl.load_workbook(xlsx_path)
    ws = wb.active

    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        for row in ws.iter_rows(values_only=True):
            writer.writerow(row)

    return csv_path


def image_to_pdf(image_paths: list, pdf_path: str = "output.pdf") -> str:
    import img2pdf
    with open(pdf_path, "wb") as f:
        f.write(img2pdf.convert(image_paths))
    return pdf_path


def text_to_pdf(text: str, pdf_path: str = "output.pdf") -> str:
    from fpdf import FPDF
    pdf = FPDF()
    pdf.add_page()
    pdf.set_auto_page_break(auto=True, margin=15)
    pdf.set_font("Courier", "", 10)
    for line in text.splitlines():
        pdf.multi_cell(0, 6, line)
    pdf.output(pdf_path)
    return pdf_path


def merge_pdfs(pdf_list: list, output_path: str = "merged.pdf") -> str:
    from tools.pdf import merge_documents
    merge_documents(pdf_list, output_path)
    return output_path


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


def pdf_to_image(pdf_path: str, output_dir: str = None, fmt: str = "png", dpi: int = 200) -> list:
    import fitz

    if output_dir is None:
        output_dir = os.path.dirname(pdf_path) or "."
    os.makedirs(output_dir, exist_ok=True)

    doc = fitz.open(pdf_path)
    base = os.path.splitext(os.path.basename(pdf_path))[0]
    paths = []

    for page_num in range(len(doc)):
        page = doc[page_num]
        pix = page.get_pixmap(dpi=dpi)
        ext = fmt.lower().replace("jpg", "jpeg")
        out_path = os.path.join(output_dir, f"{base}_page_{page_num+1}.{ext}")
        pix.save(out_path)
        paths.append(out_path)

    doc.close()
    return paths


def pdf_to_text(pdf_path: str, output_path: str = None) -> str:
    import pypdf

    if output_path is None:
        output_path = os.path.splitext(pdf_path)[0] + ".txt"

    reader = pypdf.PdfReader(pdf_path)
    lines = []
    for page in reader.pages:
        text = page.extract_text()
        if text:
            lines.append(text)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    return output_path


def pdf_to_pptx(pdf_path: str, pptx_path: str = None) -> str:
    from pptx import Presentation
    from pptx.util import Inches
    import pypdf

    if pptx_path is None:
        pptx_path = os.path.splitext(pdf_path)[0] + ".pptx"

    prs = Presentation()
    prs.slide_width = Inches(13.333)
    prs.slide_height = Inches(7.5)

    reader = pypdf.PdfReader(pdf_path)
    for page_num, page in enumerate(reader.pages):
        slide = prs.slides.add_slide(prs.slide_layouts[6])
        text = page.extract_text() or ""
        left = top = Inches(0.5)
        width = prs.slide_width - Inches(1)
        height = prs.slide_height - Inches(1)
        txBox = slide.shapes.add_textbox(left, top, width, height)
        tf = txBox.text_frame
        tf.word_wrap = True
        for i, line in enumerate(text.splitlines()):
            if line.strip():
                if i == 0:
                    tf.text = line.strip()
                else:
                    p = tf.add_paragraph()
                    p.text = line.strip()

    prs.save(pptx_path)
    return pptx_path
