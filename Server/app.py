from flask import Flask, render_template, request, jsonify, redirect, url_for
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import uuid

app = Flask(__name__)

# Database configuration
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///presence.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Database Models
class Student(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    mac_address = db.Column(db.String(17), unique=True, nullable=False)


class Class(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    uuid = db.Column(db.String(36), unique=True, default=lambda: str(uuid.uuid4()))
    name = db.Column(db.String(100), nullable=False)


class PresenceLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    mac_address = db.Column(db.String(17), nullable=False)
    class_id = db.Column(db.Integer, db.ForeignKey('class.id'), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    presence_count = db.Column(db.Integer, default=0)


class ClassStudent(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    student_id = db.Column(db.Integer, db.ForeignKey('student.id'), nullable=False)
    class_id = db.Column(db.Integer, db.ForeignKey('class.id'), nullable=False)

# Routes
@app.route('/')
def index():
    students = Student.query.all()
    classes = Class.query.all()
    return render_template('index.html', students=students, classes=classes)

@app.route('/add_student', methods=['POST'])
def add_student():
    data = request.get_json()
    name = data.get('name')
    mac_address = data.get('mac_address').upper()

    if not name or not mac_address:
        return jsonify({'error': 'Missing name or MAC address'}), 400

    existing_student = Student.query.filter_by(mac_address=mac_address).first()
    if existing_student:
        return jsonify({'error': 'Student with this MAC address already exists'}), 400

    student = Student(name=name, mac_address=mac_address)
    db.session.add(student)
    db.session.commit()

    return jsonify({'message': 'Student added successfully'}), 201


@app.route('/add_presence', methods=['POST'])
def add_presence():
    data = request.get_json()
    print(data)
    mac_address = data.get('mac_address').upper()
    class_id = data.get('class_id')

    if not mac_address or not class_id:
        return jsonify({'error': 'Missing MAC address or class ID'}), 400

    presence_log = PresenceLog.query.filter_by(mac_address=mac_address, class_id=class_id).first()

    if presence_log:
        presence_log.presence_count += 1  # Increment the count
    else:
        presence_log = PresenceLog(mac_address=mac_address, class_id=class_id, presence_count=1)
        db.session.add(presence_log)

    db.session.commit()

    return jsonify({'message': 'Presence updated successfully'}), 201


@app.route('/get_attendance/<int:class_id>', methods=['GET'])
def get_attendance(class_id):
    presences = PresenceLog.query.filter_by(class_id=class_id).all()
    attendance_counts = {presence.mac_address: presence.presence_count for presence in presences}
    return jsonify(attendance_counts)



@app.route('/assign_student', methods=['POST'])
def assign_student():
    student_id = request.form.get('student_id')  # Get form data
    class_id = request.form.get('class_id')      # Get form data

    if not student_id or not class_id:
        return jsonify({'error': 'Missing student ID or class ID'}), 400

    existing_assignment = ClassStudent.query.filter_by(student_id=student_id, class_id=class_id).first()
    if existing_assignment:
        return jsonify({'error': 'Student is already assigned to this class'}), 400

    assignment = ClassStudent(student_id=student_id, class_id=class_id)
    db.session.add(assignment)
    db.session.commit()

    return redirect(url_for('index'))  # Redirect back to the home page


@app.route('/class/<int:class_id>')
def view_class(class_id):
    class_obj = Class.query.get_or_404(class_id)
    students = (
    db.session.query(Student)
    .join(ClassStudent, Student.id == ClassStudent.student_id)
    .filter(ClassStudent.class_id == class_id)
    .all()
    )
    
    # Fetch attendance counts for each student
    attendance_counts = {
        presence.mac_address: presence.presence_count
        for presence in PresenceLog.query.filter_by(class_id=class_id).all()
    }

    return render_template('class.html', class_obj=class_obj, students=students, attendance_counts=attendance_counts)



@app.route('/add_class', methods=['POST'])
def add_class():
    class_name = request.form.get('class_name')
    if not class_name:
        return redirect(url_for('index'))
    
    new_class = Class(name=class_name)
    db.session.add(new_class)
    db.session.commit()
    
    return redirect(url_for('index'))

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(host='0.0.0.0', port=5000)
