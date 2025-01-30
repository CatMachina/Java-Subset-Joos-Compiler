import requests
import os
from bs4 import BeautifulSoup

subtitles = ["features"]

base_url = "https://student.cs.uwaterloo.ca/~cs444/joos/"

output_dir = "joos_features"
os.makedirs(output_dir, exist_ok=True)

def fetch_and_save(subtitle, feature):
    url = f"{base_url}{subtitle}/{feature}.html"
    try:
        response = requests.get(url)
        response.raise_for_status()

        soup = BeautifulSoup(response.text, "html.parser")
        content = soup.get_text()

        filename = os.path.join(output_dir, f"{subtitle}_{feature}.txt")
        with open(filename, "w", encoding="utf-8") as file:
            file.write(content)

        print(f"Saved: {filename}")

    except requests.exceptions.RequestException:
        print(f"Failed to fetch: {url}")

features = [
  "compoundnames",
  "extends",
  "implements",
  "staticmethoddeclaration",
  "classimport",
  "packageimport",
  "package",
  "interfaces",
  "staticfielddeclaration",
  "externalcall",
  "implicitthisforfields",
  "implicitthisformethods",
  "accessstaticfield",
  "callstaticmethods",
  "nonthisfieldaccess",
  "methodoverloading",
  "arraylength",
  "publicclasses",
  "protectedfields",
  "publicconstructors",
  "publicmethods",
  "publicfields",
  "protectedmethods",
  "abstract",
  "finalclasses",
  "finalmethods",
  "protectedconstructors",
  "implicitsupercall",
  "fieldinitializers",
  "constructoroverloading",
  "nestedblocks",
  "arbitraryreturn",
  "omittedvoidreturn",
  "arbitrarylocaldeclaration",
  "if",
  "while",
  "for",
  "throws",
  "boolean",
  "int",
  "char",
  "byte",
  "short",
  "array",
  "array_return",
  "intliterals",
  "booleanliterals",
  "stringliterals",
  "nullliteral",
  "this",
  "charliterals",
  "characterescapes",
  "comment",
  "javadoc",
  "arithmeticoperations",
  "comparisonoperations",
  "eagerbooleanoperations",
  "implicitstringconcatenation",
  "lazybooleanoperations",
  "instanceof",
  "primitivecasts",
  "referencecasts",
  "widening"
]

for subtitle in subtitles:
    for feature in features:
        fetch_and_save(subtitle, feature)