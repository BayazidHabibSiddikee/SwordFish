from pypdf import PdfWriter

def merge_documents(pdf_list, output_filename="merged_storyboard.pdf"):
    merger = PdfWriter()
    
    for pdf in pdf_list:
        print(f"Appending: {pdf}")
        merger.append(pdf)
        
    merger.write(output_filename)
    merger.close()
    print(f"All files successfully merged into: {output_filename}")

# Example Usage:
# files_to_merge = ["act1_script.pdf", "camera_angles_reference.pdf", "visual_style.pdf"]
# merge_documents(files_to_merge, "master_production_file.pdf")
